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

// Feed keys (from each feed's page on io.adafruit.com). Create these feeds first.
#define FEED_LINES    "nodavid-lines"   // whole list, one entry per line. Turn feed HISTORY OFF
                                        // (allows values up to 100KB instead of 1KB).
#define FEED_ADD      "nodavid-add"     // one line to append to the list
#define FEED_SHOW     "nodavid-show"    // "random" (or "1", or empty) = random line; any other text is shown as-is
#define FEED_STATUS   ""                // optional, e.g. "debug-messages": the display posts "online" here; "" = off
