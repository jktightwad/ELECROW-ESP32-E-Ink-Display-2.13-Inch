#pragma once
// Template for network credentials.
// Copy this file to include/secrets.h and fill in your details.
// include/secrets.h is listed in .gitignore, so it stays on your PC and is never pushed.
// Without a secrets.h the display still works: Wi-Fi is set up with the
// CrowPanel-Setup hotspot, and only MQTT is unavailable.

// Optional fallback network, tried after the Wi-Fi saved on the board.
// Delete these two lines to rely on the setup hotspot only.
#define WIFI_SSID     "your-network-name"     // 2.4 GHz network
#define WIFI_PASSWORD "your-wifi-password"

// Adafruit IO
#define MQTT_HOST     "io.adafruit.com"       // "" disables MQTT
#define MQTT_PORT     1883
#define MQTT_USER     "your-aio-username"
#define MQTT_PASSWORD "your-aio-key"

// Shared feed. The display acts only on JSON commands addressed to DEVICE_ID
// and silently ignores everything else on the feed:
//   {"deviceId":"nodavid","cmd":"random"}
//   {"deviceId":"nodavid","cmd":"show","text":"Back in 5"}
//   {"deviceId":"nodavid","cmd":"add","text":"A new line"}
//   {"deviceId":"nodavid","cmd":"location","text":"Albany, NY"}
#define MQTT_FEED     "onair-dot-control"
#define DEVICE_ID     "nodavid"
