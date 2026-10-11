#include "net.h"
#include "command.h"
#include <WiFi.h>
#include <WiFiManager.h>

#if __has_include("secrets.h")
#include "secrets.h"
#endif
// Defaults for settings missing from secrets.h (or with no secrets.h at all)
#ifndef MQTT_HOST
#define MQTT_HOST ""
#endif
#ifndef MQTT_PORT
#define MQTT_PORT 1883
#endif
#ifndef MQTT_USER
#define MQTT_USER ""
#endif
#ifndef MQTT_PASSWORD
#define MQTT_PASSWORD ""
#endif
#ifndef MQTT_FEED
#define MQTT_FEED "onair-dot-control"
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

bool waitForWifi(uint32_t timeout_ms) {
  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < timeout_ms) delay(100);
  return WiFi.status() == WL_CONNECTED;
}

void connectWifi(bool force_portal) {
  WiFi.mode(WIFI_STA);
  WiFi.persistent(true);
  WiFi.setAutoReconnect(true);

  if (!force_portal) {
    Serial.println("Wi-Fi: trying saved credentials...");
    WiFi.begin();
    if (waitForWifi(20000)) return;
#ifdef WIFI_SSID
    Serial.printf("Wi-Fi: trying %s from secrets.h...\n", WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    if (waitForWifi(15000)) return;
#endif
  }

  Serial.printf("Wi-Fi: opening setup hotspot \"%s\" (5 minutes)\n", AP_NAME);
  WiFiManager wm;
  wm.setConfigPortalTimeout(300);
  wm.setAPCallback([](WiFiManager*) { if (handlers.onPortal) handlers.onPortal(AP_NAME); });
  if (wm.startConfigPortal(AP_NAME)) Serial.println("Wi-Fi: set up via hotspot");
  else Serial.println("Wi-Fi: setup timed out - continuing offline, will keep retrying");
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
    case CommandType::None:     break;   // not for this device
  }
}

void connectMqtt() {
  if (mqtt.connect(client_id.c_str(), MQTT_USER, MQTT_PASSWORD)) {
    Serial.printf("MQTT connected, listening on %s\n", feed_topic.c_str());
    mqtt.subscribe(feed_topic.c_str());
    mqtt.subscribe((String(MQTT_USER) + "/errors").c_str());
    mqtt.subscribe((String(MQTT_USER) + "/throttle").c_str());
  } else {
    // State codes: see PubSubClient.h (e.g. 4 = bad username/key, 5 = not authorised)
    Serial.printf("MQTT connect failed, state %d\n", mqtt.state());
  }
}

}  // namespace

void netBegin(const NetHandlers& h, bool force_portal) {
  handlers = h;
  connectWifi(force_portal);
  if (WiFi.status() == WL_CONNECTED)
    Serial.printf("Wi-Fi connected to %s, IP %s\n", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());

  client_id = "crowpanel-" + WiFi.macAddress();
  client_id.replace(":", "");
  feed_topic = String(MQTT_USER) + "/feeds/" + MQTT_FEED;
  mqtt_enabled = strlen(MQTT_HOST) && strlen(MQTT_USER);
  if (mqtt_enabled) {
    mqtt.setServer(MQTT_HOST, MQTT_PORT);
    mqtt.setCallback(onMessage);
    mqtt.setBufferSize(2048);             // Adafruit IO values are at most ~1KB with feed history on
  } else {
    Serial.println("MQTT not configured (no Adafruit IO details in secrets.h)");
  }
}

void netLoop() {
  static bool was_connected = WiFi.status() == WL_CONNECTED;
  bool connected = WiFi.status() == WL_CONNECTED;
  if (connected != was_connected) {
    was_connected = connected;
    if (connected) Serial.printf("Wi-Fi connected, IP %s\n", WiFi.localIP().toString().c_str());
    else Serial.println("Wi-Fi disconnected");
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
}

bool netWifiConnected() { return WiFi.status() == WL_CONNECTED; }
