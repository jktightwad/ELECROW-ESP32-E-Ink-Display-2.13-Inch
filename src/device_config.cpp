#include "device_config.h"
#include <LittleFS.h>

#if __has_include("secrets.h")
#include "secrets.h"
#endif

namespace { const char* const PATH = "/config.txt"; }

DeviceConfig loadDeviceConfig() {
  DeviceConfig cfg;
  if (LittleFS.exists(PATH)) {
    File f = LittleFS.open(PATH, "r");
    while (f && f.available()) {
      String line = f.readStringUntil('\n');
      line.trim();
      int eq = line.indexOf('=');
      if (eq <= 0) continue;
      String k = line.substring(0, eq), v = line.substring(eq + 1);
      if (k == "aio_user") cfg.aio_user = v;
      else if (k == "aio_key") cfg.aio_key = v;
    }
    if (f) f.close();
  }
#ifdef MQTT_USER
  if (!cfg.aio_user.length()) cfg.aio_user = MQTT_USER;
#endif
#ifdef MQTT_PASSWORD
  if (!cfg.aio_key.length()) cfg.aio_key = MQTT_PASSWORD;
#endif
  return cfg;
}

void saveDeviceConfig(const DeviceConfig& cfg) {
  File f = LittleFS.open(PATH, "w");
  if (!f) { Serial.println("Config: could not write file"); return; }
  f.printf("aio_user=%s\naio_key=%s\n", cfg.aio_user.c_str(), cfg.aio_key.c_str());
  f.close();
  Serial.println("Config: saved");
}
