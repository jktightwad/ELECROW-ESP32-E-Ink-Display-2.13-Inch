#include "net.h"

#if __has_include("secrets.h")
#include "secrets.h"
#define HAVE_SECRETS 1
// Defaults for settings missing from an older secrets.h
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
#ifndef FEED_LINES
#define FEED_LINES "nodavid-lines"
#endif
#ifndef FEED_ADD
#define FEED_ADD "nodavid-add"
#endif
#ifndef FEED_SHOW
#define FEED_SHOW "nodavid-show"
#endif
#ifndef FEED_STATUS
#define FEED_STATUS ""
#endif
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
String client_id;
uint32_t last_attempt = 0;

String feedTopic(const char* key) { return String(MQTT_USER) + "/feeds/" + key; }

void onMessage(char* t, uint8_t* payload, unsigned int len) {
  String body;
  body.reserve(len);
  for (unsigned int i = 0; i < len; i++) body += (char)payload[i];
  String tp(t);
  Serial.printf("MQTT %s (%u bytes)\n", t, len);
  if (tp == feedTopic(FEED_LINES)) {
    handlers.onLines(body);
  } else if (tp == feedTopic(FEED_ADD)) {
    handlers.onAdd(body);
  } else if (tp == feedTopic(FEED_SHOW)) {
    String cmd = body;
    cmd.trim();
    if (cmd.length() == 0 || cmd == "1" || cmd.equalsIgnoreCase("random")) handlers.onShowRandom();
    else handlers.onShowText(body);
  }
}

void connectMqtt() {
  if (mqtt.connect(client_id.c_str(), MQTT_USER, MQTT_PASSWORD)) {
    Serial.println("MQTT connected");
    mqtt.subscribe(feedTopic(FEED_LINES).c_str());
    mqtt.subscribe(feedTopic(FEED_ADD).c_str());
    mqtt.subscribe(feedTopic(FEED_SHOW).c_str());
    // Ask Adafruit IO to resend the latest list (its alternative to retained messages).
    mqtt.publish((feedTopic(FEED_LINES) + "/get").c_str(), "");
    if (strlen(FEED_STATUS)) mqtt.publish(feedTopic(FEED_STATUS).c_str(), "nodavid display online");
  } else {
    // State codes: see PubSubClient.h (e.g. 4 = bad username/key, 5 = not authorised)
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
    // Adafruit IO limits connection attempts, so retry at most every 10 s.
    if (last_attempt == 0 || millis() - last_attempt > 10000) {
      last_attempt = millis();
      connectMqtt();
    }
    return;
  }
  mqtt.loop();
}

void netPublishLines(const String& text) {
  if (mqtt.connected()) mqtt.publish(feedTopic(FEED_LINES).c_str(), text.c_str());
}

#else  // no secrets.h

bool netBegin(const NetHandlers&) {
  Serial.println("No include/secrets.h - Wi-Fi/MQTT disabled");
  return false;
}
void netLoop() {}
void netPublishLines(const String&) {}

#endif
