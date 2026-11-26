// Public key for verifying firmware updates (ECDSA P-256, DER SubjectPublicKeyInfo).
// replace-me: run `python tools/make_ota_key.py` once to generate the real key.
// Until then this placeholder rejects every update.
#pragma once
#include <stdint.h>
#include <stddef.h>

static const uint8_t OTA_PUBLIC_KEY_DER[] = {0x00};
static const size_t OTA_PUBLIC_KEY_DER_LEN = 0;
