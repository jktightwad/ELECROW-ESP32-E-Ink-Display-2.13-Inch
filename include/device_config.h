#pragma once
#include <Arduino.h>

// Settings entered through the CrowPanel-Setup hotspot and saved in
// /config.txt on the board ("key=value" lines), so the firmware itself holds
// no credentials and release builds can be published. Like Wi-Fi passwords,
// they are stored unencrypted in flash. "Upload Filesystem Image" erases them.
struct DeviceConfig {
  String aio_user;
  String aio_key;
};

// Loads /config.txt; values missing from it fall back to secrets.h (local builds).
DeviceConfig loadDeviceConfig();
void saveDeviceConfig(const DeviceConfig& cfg);
