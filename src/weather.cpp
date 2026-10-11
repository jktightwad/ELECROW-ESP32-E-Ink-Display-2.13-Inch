#include "weather.h"
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

// Open-Meteo (https://open-meteo.com): free for non-commercial use, no API key.

namespace {

String urlEncode(const String& s) {
  String out;
  const char* hex = "0123456789ABCDEF";
  for (size_t i = 0; i < s.length(); i++) {
    uint8_t c = s[i];
    if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
      out += (char)c;
    } else {
      out += '%';
      out += hex[c >> 4];
      out += hex[c & 15];
    }
  }
  return out;
}

// GET `url` into `doc`. Returns false and sets `error` on failure.
bool getJson(const String& url, JsonDocument& doc, String& error) {
  WiFiClientSecure client;
  client.setInsecure();   // encrypted, but the server certificate is not checked
  HTTPClient http;
  http.setTimeout(10000);
  if (!http.begin(client, url)) { error = "bad URL"; return false; }
  int status = http.GET();
  if (status != HTTP_CODE_OK) {
    error = "HTTP " + String(status);
    http.end();
    return false;
  }
  DeserializationError e = deserializeJson(doc, http.getStream());
  http.end();
  if (e) { error = String("JSON: ") + e.c_str(); return false; }
  return true;
}

void copyTime(const char* iso, char out[6]) {   // "2026-10-11T07:12" -> "07:12"
  out[0] = 0;
  if (iso && strlen(iso) >= 16) { memcpy(out, iso + 11, 5); out[5] = 0; }
}

}  // namespace

bool geocode(const String& name, Location& out, String& error) {
  String url = "https://geocoding-api.open-meteo.com/v1/search?count=1&language=en&format=json&name=" +
               urlEncode(name);
  JsonDocument doc;
  if (!getJson(url, doc, error)) return false;
  JsonObject r = doc["results"][0];
  if (r.isNull()) { error = "no place found for \"" + name + "\""; return false; }
  out.lat = r["latitude"] | 0.0f;
  out.lon = r["longitude"] | 0.0f;
  out.label = name;
  out.label.trim();
  out.label.toUpperCase();
  out.valid = true;
  Serial.printf("Geocoded \"%s\" -> %s, %s (%.4f, %.4f)\n", name.c_str(), (const char*)(r["name"] | ""),
                (const char*)(r["admin1"] | ""), out.lat, out.lon);
  return true;
}

bool fetchWeather(const Location& loc, WeatherData& out, String& error) {
  char url[512];
  snprintf(url, sizeof url,
           "https://api.open-meteo.com/v1/forecast?latitude=%.4f&longitude=%.4f"
           "&current=temperature_2m,relative_humidity_2m,apparent_temperature,is_day,weather_code,"
           "wind_speed_10m,wind_direction_10m"
           "&daily=temperature_2m_max,temperature_2m_min,sunrise,sunset,precipitation_probability_max"
           "&temperature_unit=fahrenheit&wind_speed_unit=mph&timezone=auto&forecast_days=1",
           loc.lat, loc.lon);
  JsonDocument doc;
  if (!getJson(url, doc, error)) return false;
  JsonObject cur = doc["current"];
  JsonObject daily = doc["daily"];
  if (cur.isNull() || daily.isNull()) { error = "unexpected response"; return false; }

  WeatherData w;
  w.temp = cur["temperature_2m"] | 0.0f;
  w.feels = cur["apparent_temperature"] | 0.0f;
  w.humidity = cur["relative_humidity_2m"] | 0;
  w.wind = cur["wind_speed_10m"] | 0.0f;
  w.wind_dir = cur["wind_direction_10m"] | 0;
  w.code = cur["weather_code"] | -1;
  w.is_day = (cur["is_day"] | 1) != 0;
  w.high = daily["temperature_2m_max"][0] | 0.0f;
  w.low = daily["temperature_2m_min"][0] | 0.0f;
  w.precip_prob = daily["precipitation_probability_max"][0] | 0;
  copyTime(daily["sunrise"][0] | "", w.sunrise);
  copyTime(daily["sunset"][0] | "", w.sunset);
  w.utc_offset = doc["utc_offset_seconds"] | 0L;
  w.valid = true;
  out = w;
  return true;
}
