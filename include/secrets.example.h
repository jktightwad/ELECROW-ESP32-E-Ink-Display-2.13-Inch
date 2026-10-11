#pragma once
// Template for network credentials.
// Copy this file to include/secrets.h and fill in your details.
// include/secrets.h is listed in .gitignore, so it stays on your PC and is never pushed.
// Without a secrets.h the display still works (dial button only, no Wi-Fi/MQTT).

#define WIFI_SSID     "your-network-name"     // 2.4 GHz network
#define WIFI_PASSWORD "your-wifi-password"

// Adafruit IO
#define MQTT_HOST     "io.adafruit.com"       // "" disables MQTT
#define MQTT_PORT     1883
#define MQTT_USER     "your-aio-username"
#define MQTT_PASSWORD "your-aio-key"

// Shared feed: the display only acts on values starting with CMD_PREFIX and
// ignores everything else posted to the feed.
//   nodavid:random        show a random line
//   nodavid:show <text>   show <text> once
//   nodavid:add <line>    add <line> to the list stored on the board
#define MQTT_FEED     "onair-dot-control"
#define CMD_PREFIX    "nodavid:"
