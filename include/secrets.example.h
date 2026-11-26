#pragma once
// Template for network credentials.
// Copy this file to include/secrets.h and fill in your details.
// include/secrets.h is listed in .gitignore, so it stays on your PC and is never pushed.
// Without a secrets.h the display still works: Wi-Fi is set up with the
// CrowPanel-Setup hotspot, and only MQTT is unavailable.

// Optional: Wi-Fi networks to try, in addition to the ones the board has saved
// (any network set up through the CrowPanel-Setup hotspot is saved automatically).
// The strongest one in range is used. Delete this block to rely on the hotspot only.
#define WIFI_NETWORKS { \
    {"home-network", "home-password"}, \
    {"office-network", "office-password"}, \
}

// Adafruit IO username/key: normally entered once in the CrowPanel-Setup
// hotspot page and saved on the board (so release builds contain no
// credentials). Defining them here is only a fallback for builds on your PC.
// #define MQTT_USER     "your-aio-username"
// #define MQTT_PASSWORD "your-aio-key"

// Optional overrides (defaults shown):
// #define MQTT_FEED   "onair-dot-control"   // commands are read from this feed
// #define STATUS_FEED "debug-messages"      // status reports go to this feed
// #define DEVICE_ID   "nodavid"             // deviceId this display answers to
//   {"deviceId":"nodavid","cmd":"random"}
//   {"deviceId":"nodavid","cmd":"show","text":"Back in 5"}
//   {"deviceId":"nodavid","cmd":"add","text":"A new line"}
//   {"deviceId":"nodavid","cmd":"location","text":"Albany, NY"}   (or "auto": by IP address)
//   {"deviceId":"nodavid","cmd":"update"}   (check for a firmware update now)
