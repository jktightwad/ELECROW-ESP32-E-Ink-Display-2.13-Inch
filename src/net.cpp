#include "net.h"

#if __has_include("secrets.h")
#include "secrets.h"
#define HAVE_SECRETS 1
#else
#define HAVE_SECRETS 0
#endif

#if HAVE_SECRETS
#include <WiFi.h>
#include <PubSubClient.h>

namespace {

WiFiClient wifi_client;
PubSubClient mqtt(wifi_client);
NetHandlers handlers;
String prefix = MQTT_TOPIC_PREFIX;
String client_id;
uint32_t last_attempt = 0;

String topic(const char* suffix) { return prefix + suffix; }

void onMessage(char* t, uint8_t* payload, unsigned int len) {
  String body;
  body.reserve(len);
  for (unsigned int i = 0; i < len; i++) body += (char)payload[i];
  String tp(t);
  Serial.printf("MQTT %s (%u bytes)\n", t, len);
  if (tp == topic("/lines")) handlers.onLines(body);
  else if (tp == topic("/lines/add")) handlers.onAdd(body);
  else if (tp == topic("/show/random")) handlers.onShowRandom();
  else if (tp == topic("/show/text")) handlers.onShowText(body);
}

void connectMqtt() {
  const char* user = strlen(MQTT_USER) ? MQTT_USER : nullptr;
  const char* pass = strlen(MQTT_USER) ? MQTT_PASSWORD : nullptr;
  String status = topic("/status");
  // Last will: the broker publishes "offline" if the display drops off.
  if (mqtt.connect(client_id.c_str(), user, pass, status.c_str(), 1, true, "offline")) {
    Serial.println("MQTT connected");
    mqtt.publish(status.c_str(), "online", true);
    mqtt.subscribe(topic("/lines").c_str());
    mqtt.subscribe(topic("/lines/add").c_str());
    mqtt.subscribe(topic("/show/random").c_str());
    mqtt.subscribe(topic("/show/text").c_str());
  } else {
    Serial.printf("MQTT connect failed, state %d\n", mqtt.state());
  }
}

}  // namespace

bool netBegin(const NetHandlers& h) {
  handlers = h;
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);   // connects in the background
  client_id = "crowpanel-" + WiFi.macAddress();
  client_id.replace(":", "");
  if (strlen(MQTT_HOST)) {
    mqtt.setServer(MQTT_HOST, MQTT_PORT);
    mqtt.setCallback(onMessage);
    mqtt.setBufferSize(8192);             // whole list arrives as one message
  }
  Serial.printf("Wi-Fi: connecting to %s\n", WIFI_SSID);
  return true;
}

void netLoop() {
  static bool was_connected = false;
  bool connected = WiFi.status() == WL_CONNECTED;
  if (connected != was_connected) {
    was_connected = connected;
    if (connected) Serial.printf("Wi-Fi connected, IP %s\n", WiFi.localIP().toString().c_str());
    else Serial.println("Wi-Fi disconnected");
  }
  if (!connected || !strlen(MQTT_HOST)) return;
  if (!mqtt.connected()) {
    if (millis() - last_attempt > 5000) {  // retry every 5 s
      last_attempt = millis();
      connectMqtt();
    }
    return;
  }
  mqtt.loop();
}

void netPublishLines(const String& text) {
  if (mqtt.connected()) mqtt.publish(topic("/lines").c_str(), text.c_str(), true);
}

#else  // no secrets.h

bool netBegin(const NetHandlers&) {
  Serial.println("No include/secrets.h - Wi-Fi/MQTT disabled");
  return false;
}
void netLoop() {}
void netPublishLines(const String&) {}

#endif
