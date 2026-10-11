#include "command.h"
#include <ArduinoJson.h>

namespace {

bool sameId(const char* a, const char* b) {
  return a && b && strcasecmp(a, b) == 0;
}

bool isForDevice(JsonObjectConst obj, const char* device_id) {
  if (sameId(obj["deviceId"] | (const char*)nullptr, device_id)) return true;
  for (JsonVariantConst v : obj["deviceIds"].as<JsonArrayConst>()) {
    if (sameId(v.as<const char*>(), device_id)) return true;
  }
  return false;
}

}  // namespace

Command parseCommand(const char* payload, size_t len, const char* device_id) {
  Command out;
  JsonDocument doc;
  if (deserializeJson(doc, payload, len)) return out;   // not JSON: not for us

  // Unwrap {"value":"{...}"} if present.
  JsonDocument inner;
  JsonObjectConst obj;
  if (doc["value"].is<const char*>()) {
    if (deserializeJson(inner, doc["value"].as<const char*>())) return out;
    obj = inner.as<JsonObjectConst>();
  } else {
    obj = doc.as<JsonObjectConst>();
  }
  if (obj.isNull() || !isForDevice(obj, device_id)) return out;

  const char* cmd = obj["cmd"] | "";
  const char* text = obj["text"] | "";
  if (strcasecmp(cmd, "random") == 0) {
    out.type = CommandType::Random;
  } else if (strcasecmp(cmd, "show") == 0 || strcasecmp(cmd, "add") == 0) {
    if (!*text) { out.error = String("\"") + cmd + "\" needs a \"text\" value"; return out; }
    out.type = (strcasecmp(cmd, "show") == 0) ? CommandType::Show : CommandType::Add;
    out.text = text;
  } else {
    out.error = String("unknown cmd \"") + cmd + "\" (use random, show, add)";
  }
  return out;
}
