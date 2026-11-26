#include "net.h"
#include "command.h"
#include <WiFi.h>
#include <WiFiMulti.h>
#include <WiFiManager.h>
#include <esp_wifi.h>
#include "wifi_store.h"
#include "device_config.h"

#if __has_include("secrets.h")
#include "secrets.h"
#endif
// Adafruit IO username/key come from the board's saved settings (device_config.h),
// entered through the setup hotspot. These are fixed per build.
#ifndef MQTT_HOST
#define MQTT_HOST "io.adafruit.com"
#endif
#ifndef MQTT_PORT
#define MQTT_PORT 1883
#endif
#ifndef MQTT_FEED
#define MQTT_FEED "onair-dot-control"
#endif
#ifndef STATUS_FEED
#define STATUS_FEED "debug-messages"   // existing shared feed the status reports go to
#endif
#ifndef DEVICE_ID
#define DEVICE_ID "nodavid"
#endif

#include <PubSubClient.h>

namespace {

const char* const AP_NAME = "CrowPanel-Setup";

WiFiClient wifi_client;
PubSubClient mqtt(wifi_client);
NetHandlers handlers;
String client_id;
String feed_topic;
uint32_t last_attempt = 0;
bool mqtt_enabled = false;
DeviceConfig config;
String status_topic;
std::vector<String> status_queue;     // status reports waiting for an MQTT connection

WiFiMulti wifi_multi;
WifiStore wifi_store;
uint32_t wifi_lost_at = 0;
uint32_t last_wifi_retry = 0;

#ifdef WIFI_NETWORKS
struct BuiltinNet { const char* ssid; const char* pass; };
const BuiltinNet BUILTIN_NETWORKS[] = WIFI_NETWORKS;
#endif

// Networks to try: saved ones (most recent first), then those in secrets.h.
bool known_added = false;
void addKnownNetworks() {
  if (known_added) return;
  known_added = true;
  for (const WifiCred& n : wifi_store.networks()) wifi_multi.addAP(n.ssid.c_str(), n.pass.c_str());
#ifdef WIFI_NETWORKS
  for (const BuiltinNet& n : BUILTIN_NETWORKS) wifi_multi.addAP(n.ssid, n.pass);
#endif
#ifdef WIFI_SSID
  wifi_multi.addAP(WIFI_SSID, WIFI_PASSWORD);
#endif
}

// The network the ESP32 itself saved (from earlier firmware or WiFiManager),
// copied into our list so it isn't lost.
void importEspSavedNetwork() {
  wifi_config_t conf;
  if (esp_wifi_get_config(WIFI_IF_STA, &conf) != ESP_OK) return;
  String ssid = String((const char*)conf.sta.ssid);
  if (!ssid.length()) return;
  bool known = false;
  for (const WifiCred& n : wifi_store.networks()) known |= (n.ssid == ssid);
  if (!known) wifi_store.remember(ssid, String((const char*)conf.sta.password));
}

// Records the network we are connected to as the most recently used.
void rememberCurrentNetwork() {
  if (WiFi.status() == WL_CONNECTED) wifi_store.remember(WiFi.SSID(), WiFi.psk());
}

void connectWifi(bool force_portal) {
  WiFi.mode(WIFI_STA);
  WiFi.persistent(true);
  WiFi.setAutoReconnect(true);
  wifi_store.load();
  importEspSavedNetwork();
  // Builds from your PC: save the networks in secrets.h on the board, so
  // release builds (which have no secrets.h) know them too.
#ifdef WIFI_NETWORKS
  for (const BuiltinNet& n : BUILTIN_NETWORKS) wifi_store.addIfMissing(n.ssid, n.pass);
#endif
#ifdef WIFI_SSID
  wifi_store.addIfMissing(WIFI_SSID, WIFI_PASSWORD);
#endif

  if (!force_portal) {
    addKnownNetworks();
    Serial.printf("Wi-Fi: scanning for %u known network(s)...\n", (unsigned)wifi_store.networks().size());
    // Connects to the strongest known network in range; a few attempts covers a slow router.
    for (int attempt = 0; attempt < 3 && WiFi.status() != WL_CONNECTED; attempt++) {
      wifi_multi.run(10000);
    }
    if (WiFi.status() == WL_CONNECTED) {
      rememberCurrentNetwork();
      return;
    }
  }

  Serial.printf("Wi-Fi: opening setup hotspot \"%s\" (5 minutes)\n", AP_NAME);
  WiFiManager wm;
  wm.setConfigPortalTimeout(300);
  wm.setBreakAfterConfig(true);          // return after "Save" even if the network fails, so settings are kept
  wm.setAPCallback([](WiFiManager*) { if (handlers.onPortal) handlers.onPortal(AP_NAME); });
  // Adafruit IO details, on the same page as the network choice. The key field
  // starts empty (the hotspot is open), and empty means "keep the saved key".
  WiFiManagerParameter p_user("aio_user", "Adafruit IO username", config.aio_user.c_str(), 40);
  WiFiManagerParameter p_key("aio_key", "Adafruit IO key (leave empty to keep the saved one)", "", 64);
  wm.addParameter(&p_user);
  wm.addParameter(&p_key);
  bool ok = wm.startConfigPortal(AP_NAME);

  String user = String(p_user.getValue()), key = String(p_key.getValue());
  user.trim();
  key.trim();
  if (user != config.aio_user || key.length()) {
    config.aio_user = user;
    if (key.length()) config.aio_key = key;
    saveDeviceConfig(config);
  }

  addKnownNetworks();                    // for later reconnects
  if (ok && WiFi.status() == WL_CONNECTED) {
    Serial.println("Wi-Fi: set up via hotspot");
    rememberCurrentNetwork();
    wifi_multi.addAP(WiFi.SSID().c_str(), WiFi.psk().c_str());
  } else {
    Serial.println("Wi-Fi: not connected after setup - continuing offline, will keep retrying");
  }
}

void onMessage(char* t, uint8_t* payload, unsigned int len) {
  String body;
  body.reserve(len);
  for (unsigned int i = 0; i < len; i++) body += (char)payload[i];
  body.trim();

  // Adafruit IO reports rejected publishes/subscribes, throttling and bans here.
  String tp(t);
  if (tp.endsWith("/errors") || tp.endsWith("/throttle")) {
    Serial.printf("[AIO] %s: %s\n", t, body.c_str());
    return;
  }

  Command c = parseCommand(body.c_str(), body.length(), DEVICE_ID);
  if (c.error.length()) Serial.printf("MQTT command error: %s\n", c.error.c_str());
  switch (c.type) {
    case CommandType::Random:   Serial.println("MQTT: random");   handlers.onShowRandom(); break;
    case CommandType::Show:     Serial.println("MQTT: show");     handlers.onShowText(c.text); break;
    case CommandType::Add:      Serial.println("MQTT: add");      handlers.onAdd(c.text); break;
    case CommandType::Location: Serial.println("MQTT: location"); handlers.onLocation(c.text); break;
    case CommandType::Update:   Serial.println("MQTT: update");   handlers.onUpdate(); break;
    case CommandType::None:     break;   // not for this device
  }
}

void connectMqtt() {
  if (mqtt.connect(client_id.c_str(), config.aio_user.c_str(), config.aio_key.c_str())) {
    Serial.printf("MQTT connected, listening on %s\n", feed_topic.c_str());
    mqtt.subscribe(feed_topic.c_str());
    mqtt.subscribe((config.aio_user + "/errors").c_str());
    mqtt.subscribe((config.aio_user + "/throttle").c_str());
  } else {
    // State codes: see PubSubClient.h (e.g. 4 = bad username/key, 5 = not authorised)
    Serial.printf("MQTT connect failed, state %d\n", mqtt.state());
  }
}

}  // namespace

void netBegin(const NetHandlers& h, bool force_portal) {
  handlers = h;
  config = loadDeviceConfig();
  connectWifi(force_portal);
  if (WiFi.status() == WL_CONNECTED)
    Serial.printf("Wi-Fi connected to %s, IP %s\n", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());

  client_id = "crowpanel-" + WiFi.macAddress();
  client_id.replace(":", "");
  feed_topic = config.aio_user + "/feeds/" + MQTT_FEED;
  status_topic = config.aio_user + "/feeds/" + STATUS_FEED;
  mqtt_enabled = strlen(MQTT_HOST) && config.aio_user.length() && config.aio_key.length();
  if (mqtt_enabled) {
    mqtt.setServer(MQTT_HOST, MQTT_PORT);
    mqtt.setCallback(onMessage);
    mqtt.setBufferSize(2048);             // Adafruit IO values are at most ~1KB with feed history on
  } else {
    Serial.println("MQTT not configured (no Adafruit IO details: enter them in the setup hotspot)");
  }
}

void netLoop() {
  static bool was_connected = WiFi.status() == WL_CONNECTED;
  bool connected = WiFi.status() == WL_CONNECTED;
  if (connected != was_connected) {
    was_connected = connected;
    if (connected) {
      Serial.printf("Wi-Fi connected to %s, IP %s\n", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
      rememberCurrentNetwork();
    } else {
      Serial.println("Wi-Fi disconnected");
      wifi_lost_at = millis();
    }
  }
  // If the network stays down, look for any other known network (every minute).
  if (!connected && millis() - wifi_lost_at > 60000 && millis() - last_wifi_retry > 60000) {
    last_wifi_retry = millis();
    Serial.println("Wi-Fi: still down - scanning for known networks");
    wifi_multi.run(10000);
    connected = WiFi.status() == WL_CONNECTED;
  }
  if (!connected || !mqtt_enabled) return;
  if (!mqtt.connected()) {
    // Adafruit IO limits connection attempts, so retry at most every 10 s.
    if (last_attempt == 0 || millis() - last_attempt > 10000) {
      last_attempt = millis();
      connectMqtt();
    }
    return;
  }
  mqtt.loop();
  // Send queued status reports (Adafruit IO allows ~30 publishes a minute per account).
  static uint32_t last_status = 0;
  if (!status_queue.empty() && millis() - last_status > 5000) {
    last_status = millis();
    if (mqtt.publish(status_topic.c_str(), status_queue.front().c_str())) {
      Serial.printf("Status sent: %s\n", status_queue.front().c_str());
      status_queue.erase(status_queue.begin());
    }
  }
}

void netReportStatus(const String& json) {
  if (!mqtt_enabled) return;
  if (status_queue.size() >= 8) status_queue.erase(status_queue.begin());
  status_queue.push_back(json);
}

bool netWifiConnected() { return WiFi.status() == WL_CONNECTED; }
bool netMqttEnabled() { return mqtt_enabled; }
bool netMqttConnected() { return mqtt_enabled && mqtt.connected(); }

String netSsid() { return WiFi.status() == WL_CONNECTED ? WiFi.SSID() : String(""); }
int netRssi() { return WiFi.status() == WL_CONNECTED ? WiFi.RSSI() : 0; }
String netIp() { return WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : String(""); }
const char* netMqttStatus() {
  if (!mqtt_enabled) return "not set up";
  if (WiFi.status() != WL_CONNECTED) return "offline";
  return mqtt.connected() ? "connected" : "connecting";
}
const char* netDeviceId() { return DEVICE_ID; }
String netMac() { return WiFi.macAddress(); }
