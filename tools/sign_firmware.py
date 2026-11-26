#!/usr/bin/env python3
"""Signs a firmware image and writes the OTA manifest (used by the release workflow).

  OTA_SIGNING_KEY=<hex> python tools/sign_firmware.py firmware.bin 1.3.0 <download-url> manifest.json

Needs: pip install cryptography
"""
import base64, hashlib, json, os, sys
from cryptography.hazmat.primitives import hashes
from cryptography.hazmat.primitives.asymmetric import ec


def main():
    image_path, version, url, manifest_path = sys.argv[1:5]
    key_hex = os.environ.get("OTA_SIGNING_KEY", "").strip()
    if len(key_hex) != 64:
        sys.exit("OTA_SIGNING_KEY is missing or not 64 hex characters")
    key = ec.derive_private_key(int(key_hex, 16), ec.SECP256R1())
    data = open(image_path, "rb").read()
    signature = key.sign(data, ec.ECDSA(hashes.SHA256()))       # DER, over SHA-256 of the image
    manifest = {
        "version": version,
        "url": url,
        "size": len(data),
        "sha256": hashlib.sha256(data).hexdigest(),
        "signature": base64.b64encode(signature).decode(),
    }
    with open(manifest_path, "w") as f:
        json.dump(manifest, f, indent=2)
    print(json.dumps({k: v for k, v in manifest.items() if k != "signature"}, indent=2))


if __name__ == "__main__":
    main()
