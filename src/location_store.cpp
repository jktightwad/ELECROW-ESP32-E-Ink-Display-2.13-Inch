#include "location_store.h"
#include <LittleFS.h>

namespace { const char* const PATH = "/location.txt"; }

bool loadLocation(Location& out) {
  if (!LittleFS.exists(PATH)) return false;
  File f = LittleFS.open(PATH, "r");
  if (!f) return false;
  String label = f.readStringUntil('\n');
  String lat = f.readStringUntil('\n');
  String lon = f.readStringUntil('\n');
  f.close();
  label.trim();
  if (!label.length() || !lat.length() || !lon.length()) return false;
  out.label = label;
  out.lat = lat.toFloat();
  out.lon = lon.toFloat();
  out.valid = true;
  return true;
}

void saveLocation(const Location& loc) {
  File f = LittleFS.open(PATH, "w");
  if (!f) return;
  f.printf("%s\n%.4f\n%.4f\n", loc.label.c_str(), loc.lat, loc.lon);
  f.close();
}
