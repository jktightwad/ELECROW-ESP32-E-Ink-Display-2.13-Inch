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
#ifndef MQTT_FEED
#define MQTT_FEED "onair-dot-control"
#endif
#ifndef CMD_PREFIX
#define CMD_PREFIX "nodavid:"
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
String feed_topic;
uint32_t last_attempt = 0;

void onMessage(char* t, uint8_t* payload, unsigned int len) {
  String body;
  body.reserve(len);
  for (unsigned int i = 0; i < len; i++) body += (char)payload[i];
  body.trim();
  if (!body.startsWith(CMD_PREFIX)) return;   // meant for another device on the shared feed

  String cmd = body.substring(strlen(CMD_PREFIX));
  cmd.trim();
  Serial.printf("MQTT command: %s\n", cmd.c_str());
  if (cmd.equalsIgnoreCase("random")) {
    handlers.onShowRandom();
  } else if (cmd.startsWith("show ")) {
    handlers.onShowText(cmd.substring(5));
  } else if (cmd.startsWith("add ")) {
    handlers.onAdd(cmd.substring(4));
  } else {
    Serial.println("  unknown command (use random, show <text>, add <line>)");
  }
}

void connectMqtt() {
  if (mqtt.connect(client_id.c_str(), MQTT_USER, MQTT_PASSWORD)) {
    Serial.printf("MQTT connected, listening on %s\n", feed_topic.c_str());
    mqtt.subscribe(feed_topic.c_str());
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
  feed_topic = String(MQTT_USER) + "/feeds/" + MQTT_FEED;
  if (strlen(MQTT_HOST)) {
    mqtt.setServer(MQTT_HOST, MQTT_PORT);
    mqtt.setCallback(onMessage);
    mqtt.setBufferSize(2048);             // Adafruit IO values are at most ~1KB with feed history on
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

#else  // no secrets.h

bool netBegin(const NetHandlers&) {
  Serial.println("No include/secrets.h - Wi-Fi/MQTT disabled");
  return false;
}
void netLoop() {}

#endif
