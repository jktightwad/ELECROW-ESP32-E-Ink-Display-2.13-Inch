#pragma once
// Template for network credentials.
// Copy this file to include/secrets.h and fill in your details.
// include/secrets.h is listed in .gitignore, so it stays on your PC and is never pushed.
// Without a secrets.h the display still works (dial button only, no Wi-Fi/MQTT).

#define WIFI_SSID     "your-network-name"     // 2.4 GHz network
#define WIFI_PASSWORD "your-wifi-password"

#define MQTT_HOST     "192.168.1.10"          // broker IP or hostname; "" disables MQTT
#define MQTT_PORT     1883
#define MQTT_USER     ""                      // "" if the broker needs no login
#define MQTT_PASSWORD ""
#define MQTT_TOPIC_PREFIX "desk"              // topics: desk/lines, desk/show/random, ...
