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

// Loads /config.txt. Values missing from it are taken from secrets.h (builds
// from your PC) and saved, so later release builds have them too.
DeviceConfig loadDeviceConfig();
void saveDeviceConfig(const DeviceConfig& cfg);
