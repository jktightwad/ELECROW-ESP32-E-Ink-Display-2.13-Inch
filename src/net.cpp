#include "net.h"
#include "command.h"

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
#ifndef DEVICE_ID
#define DEVICE_ID "nodavid"
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

  // Adafruit IO reports rejected publishes/subscribes, throttling and bans here.
  String tp(t);
  if (tp.endsWith("/errors") || tp.endsWith("/throttle")) {
    Serial.printf("[AIO] %s: %s\n", t, body.c_str());
    return;
  }
  Command c = parseCommand(body.c_str(), body.length(), DEVICE_ID);
  if (c.error.length()) Serial.printf("MQTT command error: %s\n", c.error.c_str());
  switch (c.type) {
    case CommandType::Random: Serial.println("MQTT: random"); handlers.onShowRandom(); break;
    case CommandType::Show:   Serial.println("MQTT: show");   handlers.onShowText(c.text); break;
    case CommandType::Add:    Serial.println("MQTT: add");    handlers.onAdd(c.text); break;
    case CommandType::None:   break;   // not for this device
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

NetState netWifiState() {
  return WiFi.status() == WL_CONNECTED ? NetState::Connected : NetState::Connecting;
}

NetState netMqttState() {
  if (!strlen(MQTT_HOST)) return NetState::Off;
  return mqtt.connected() ? NetState::Connected : NetState::Connecting;
}

String netIp() {
  return WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : String("");
}

#else  // no secrets.h

bool netBegin(const NetHandlers&) {
  Serial.println("No include/secrets.h - Wi-Fi/MQTT disabled");
  return false;
}
void netLoop() {}
NetState netWifiState() { return NetState::Off; }
NetState netMqttState() { return NetState::Off; }
String netIp() { return String(""); }

#endif
