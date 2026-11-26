#include "ota.h"
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <LittleFS.h>
#include <Update.h>
#include <WiFiClientSecure.h>
#include <esp_ota_ops.h>
#include <mbedtls/base64.h>
#include <mbedtls/pk.h>
#include <mbedtls/md.h>
#include "ota_public_key.h"

#ifndef OTA_MANIFEST_URL
#define OTA_MANIFEST_URL \
  "https://github.com/jktightwad/ELECROW-ESP32-E-Ink-Display-2.13-Inch/releases/latest/download/manifest.json"
#endif

namespace {

const char* const PENDING_PATH = "/ota_pending.txt";   // previous version, while a new one is unconfirmed
const char* const SKIP_PATH = "/ota_skip.txt";         // version that was rolled back
const char* const RESULT_PATH = "/ota_result.txt";     // message for the firmware we rolled back to
const uint32_t OTA_VALIDATE_MS = 5 * 60000UL;
const uint32_t OTA_CONFIRM_AFTER_MS = 60000UL;         // must stay up this long once online
const int MAX_UNCONFIRMED_BOOTS = 3;

// Survives restarts (not power loss); the magic value tells a real count from garbage.
RTC_NOINIT_ATTR uint32_t rtc_magic;
RTC_NOINIT_ATTR uint32_t rtc_boots;
const uint32_t MAGIC = 0x0DA71D01;

String readFile(const char* path) {
  if (!LittleFS.exists(path)) return "";
  File f = LittleFS.open(path, "r");
  String s = f ? f.readString() : "";
  if (f) f.close();
  s.trim();
  return s;
}

void writeFile(const char* path, const String& s) {
  File f = LittleFS.open(path, "w");
  if (f) { f.print(s); f.close(); }
}

String event(const char* name, const String& detail) {
  JsonDocument d;
  d["type"] = "nodavid-status";
  d["fw"] = FW_VERSION;
  d["event"] = name;
  if (detail.length()) d["detail"] = detail;
  String out;
  serializeJson(d, out);
  return out;
}

// Switches to the other app slot (the previous firmware) and restarts.
void rollBack(const String& why) {
  String previous = readFile(PENDING_PATH);
  Serial.printf("OTA: rolling back to %s (%s)\n", previous.c_str(), why.c_str());
  writeFile(SKIP_PATH, FW_VERSION);
  writeFile(RESULT_PATH, String("rolled back from ") + FW_VERSION + ": " + why);
  LittleFS.remove(PENDING_PATH);
  rtc_magic = 0;
  const esp_partition_t* other = esp_ota_get_next_update_partition(nullptr);
  if (other && esp_ota_set_boot_partition(other) == ESP_OK) {
    delay(200);
    ESP.restart();
  }
  Serial.println("OTA: rollback failed - no previous firmware slot");
}

bool verifySignature(const uint8_t hash[32], const String& sig_b64, String& error) {
  if (OTA_PUBLIC_KEY_DER_LEN == 0) { error = "no public key in firmware (run tools/make_ota_key.py)"; return false; }
  uint8_t sig[128];
  size_t sig_len = 0;
  if (mbedtls_base64_decode(sig, sizeof sig, &sig_len, (const uint8_t*)sig_b64.c_str(), sig_b64.length()) != 0) {
    error = "bad signature encoding";
    return false;
  }
  mbedtls_pk_context pk;
  mbedtls_pk_init(&pk);
  int rc = mbedtls_pk_parse_public_key(&pk, OTA_PUBLIC_KEY_DER, OTA_PUBLIC_KEY_DER_LEN);
  if (rc == 0) rc = mbedtls_pk_verify(&pk, MBEDTLS_MD_SHA256, hash, 32, sig, sig_len);
  mbedtls_pk_free(&pk);
  if (rc != 0) { error = "signature check failed"; return false; }
  return true;
}

}  // namespace

bool versionNewer(const String& a, const String& b) {
  int pa[3] = {0, 0, 0}, pb[3] = {0, 0, 0};
  sscanf(a.c_str(), "%d.%d.%d", &pa[0], &pa[1], &pa[2]);
  sscanf(b.c_str(), "%d.%d.%d", &pb[0], &pb[1], &pb[2]);
  for (int i = 0; i < 3; i++) {
    if (pa[i] != pb[i]) return pa[i] > pb[i];
  }
  return false;
}

String otaBootCheck() {
  String result = readFile(RESULT_PATH);
  if (result.length()) {                 // we are the firmware an update rolled back to
    LittleFS.remove(RESULT_PATH);
    return event("ota-rolled-back", result);
  }
  if (!LittleFS.exists(PENDING_PATH)) {
    rtc_magic = 0;
    return "";
  }
  if (rtc_magic != MAGIC) { rtc_magic = MAGIC; rtc_boots = 0; }
  rtc_boots++;
  Serial.printf("OTA: unconfirmed update, start %u of %d\n", (unsigned)rtc_boots, MAX_UNCONFIRMED_BOOTS);
  if (rtc_boots > (uint32_t)MAX_UNCONFIRMED_BOOTS) rollBack("kept restarting");
  return "";
}

String otaLoop(bool online) {
  static uint32_t online_since = 0;
  static uint32_t started = millis();    // first call: end of setup
  static bool pending = LittleFS.exists(PENDING_PATH);
  if (!pending) return "";
  if (!online) online_since = 0;
  else if (!online_since) online_since = millis();
  if (online_since && millis() - online_since >= OTA_CONFIRM_AFTER_MS) {
    String previous = readFile(PENDING_PATH);
    LittleFS.remove(PENDING_PATH);
    pending = false;
    rtc_magic = 0;
    Serial.printf("OTA: update to %s confirmed\n", FW_VERSION);
    return event("ota-updated", "from " + previous);
  }
  if (millis() - started > OTA_VALIDATE_MS) rollBack("could not get online");
  return "";
}

OtaResult otaCheck(bool force, String& message, void (*progress)(const char*)) {
  // 1. Manifest (GitHub redirects "latest" to the release's file).
  WiFiClientSecure client;
  client.setInsecure();   // authenticity comes from the firmware signature, not TLS
  HTTPClient http;
  http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
  http.setTimeout(15000);
  http.useHTTP10(true);
  if (!http.begin(client, OTA_MANIFEST_URL)) { message = "bad manifest URL"; return OtaResult::Failed; }
  int status = http.GET();
  if (status != HTTP_CODE_OK) {
    http.end();
    message = status == 404 ? "no release published yet" : "manifest HTTP " + String(status);
    return status == 404 ? OtaResult::UpToDate : OtaResult::Failed;
  }
  JsonDocument m;
  DeserializationError je = deserializeJson(m, http.getString());
  http.end();
  if (je) { message = "manifest JSON: " + String(je.c_str()); return OtaResult::Failed; }
  String version = m["version"] | "";
  String url = m["url"] | "";
  String sig = m["signature"] | "";
  size_t size = m["size"] | 0;
  if (!version.length() || !url.length() || !sig.length() || !size) { message = "manifest incomplete"; return OtaResult::Failed; }

  // 2. Is it newer?
  bool dev = String(FW_VERSION) == "dev";
  if (!force && dev) { message = "dev build: updates only on the update command"; return OtaResult::Skipped; }
  if (!force && version == readFile(SKIP_PATH)) { message = version + " was rolled back; skipping"; return OtaResult::Skipped; }
  if (!dev && !versionNewer(version, FW_VERSION)) { message = "up to date (" + String(FW_VERSION) + ")"; return OtaResult::UpToDate; }

  // 3. Download into the spare slot, hashing as we go.
  Serial.printf("OTA: downloading %s (%u bytes)\n", version.c_str(), (unsigned)size);
  if (progress) progress(("Downloading " + version + "...").c_str());
  if (!http.begin(client, url)) { message = "bad firmware URL"; return OtaResult::Failed; }
  status = http.GET();
  if (status != HTTP_CODE_OK) { http.end(); message = "firmware HTTP " + String(status); return OtaResult::Failed; }
  if (!Update.begin(size)) { http.end(); message = "no room: " + String(Update.errorString()); return OtaResult::Failed; }
  mbedtls_md_context_t sha;
  mbedtls_md_init(&sha);
  mbedtls_md_setup(&sha, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 0);
  mbedtls_md_starts(&sha);
  auto* stream = http.getStreamPtr();
  uint8_t buf[2048];
  size_t done = 0;
  uint32_t last_data = millis();
  while (done < size && millis() - last_data < 20000) {
    size_t avail = stream->available();
    if (!avail) { delay(5); continue; }
    int n = stream->readBytes(buf, min(avail, sizeof buf));
    if (n <= 0) continue;
    mbedtls_md_update(&sha, buf, n);
    if (Update.write(buf, n) != (size_t)n) break;
    done += n;
    last_data = millis();
  }
  http.end();
  uint8_t hash[32];
  mbedtls_md_finish(&sha, hash);
  mbedtls_md_free(&sha);
  if (done != size) { Update.abort(); message = "download incomplete (" + String(done) + "/" + String(size) + ")"; return OtaResult::Failed; }

  // 4. Signature, then switch slots.
  String error;
  if (!verifySignature(hash, sig, error)) { Update.abort(); message = error; return OtaResult::Failed; }
  if (!Update.end(true)) { message = "install: " + String(Update.errorString()); return OtaResult::Failed; }
  writeFile(PENDING_PATH, FW_VERSION);
  LittleFS.remove(SKIP_PATH);
  rtc_magic = 0;
  message = "installed " + version + ", restarting";
  return OtaResult::Installed;
}

String otaEvent(const char* name, const String& detail) { return event(name, detail); }
