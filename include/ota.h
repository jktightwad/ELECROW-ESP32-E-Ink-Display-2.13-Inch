#pragma once
#include <Arduino.h>

// Over-the-air updates from GitHub Releases.
//
// The release workflow (.github/workflows/release.yml) builds the firmware,
// signs it (ECDSA P-256, key in the OTA_SIGNING_KEY GitHub secret) and
// publishes firmware.bin + manifest.json. The board reads the latest
// manifest, and if the version is newer it downloads the image into the spare
// app slot, checks the signature against include/ota_public_key.h, and
// restarts into it. Saved data (file system) is not touched.
//
// Safety net: after an update the new firmware must hold a Wi-Fi connection
// for a minute within 5 minutes of starting, without restarting more than a
// few times; otherwise it switches back to the previous firmware, which then skips
// that version until an "update" command forces it.

#ifndef FW_VERSION
#define FW_VERSION "dev"        // release builds set this from the git tag
#endif

enum class OtaResult { UpToDate, Skipped, Failed, Installed };

// Call first thing in setup(): counts restarts of an unconfirmed update and
// rolls back if it keeps failing. Returns a status event to report, or "".
String otaBootCheck();

// Call regularly: confirms a new firmware once it is online, or rolls back
// after the time limit. Returns a status event to report, or "".
String otaLoop(bool online);

// Checks for an update and installs it. `force` (MQTT "update" command) also
// updates "dev" builds and versions that were previously rolled back.
// `progress` is called with a short message for the screen. On Installed the
// caller should report and restart.
OtaResult otaCheck(bool force, String& message, void (*progress)(const char*));

// a > b for "x.y.z" version strings.
bool versionNewer(const String& a, const String& b);

// JSON status report {"type":"nodavid-status","fw":...,"event":...,"detail":...}.
String otaEvent(const char* name, const String& detail);
