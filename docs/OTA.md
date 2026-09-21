# OTA Update Design

This document describes the bounded OTA implementation for Raster Pick to Light.
The goal is safe, deterministic updates without claiming production-grade firmware
security on unverified hardware or bootloaders.

## Scope and constraints

- Flash and bootloader implementation must be board-provided and verified.
- The default target backend is fail-closed.
- This repository does not include a production signature backend or secure key store.
- Any real update requires a signed image and a verified bootloader/flash implementation.

## Required board contract

The actual bootloader, flash API, and signature validation backend must provide these guarantees:

1. A dedicated staging slot or writable region separate from the active firmware image.
2. A write-only staging region that cannot overwrite the active slot directly.
3. A board API that accepts metadata: target, version, length, digest, and a required signature.
4. A bootloader that only commits the staged image after health checks succeed.
5. A guaranteed rollback or recovery path if the image fails verification or health checks.
6. A signature verifier tied to a trusted hardware root or an explicitly configured board backend.

When these are not available, the firmware must refuse to apply the update.

## State machine

The OTA logic enforces this sequence:

- AUTH_REQUIRED -> READY
- READY -> RECEIVING
- RECEIVING -> STAGING
- STAGING -> PENDING
- PENDING -> HEALTH_CONFIRM
- HEALTH_CONFIRM -> APPLIED
- Any stage -> FAILED or ABORTED on verification failure or interrupted upload

The implementation intentionally refuses to overwrite the active firmware. The staging abstraction is required before the pending image is committed.

## Metadata requirements

Each image package must include:

- target: board model or firmware target, such as rp2354
- version: semantic version string
- length: exact firmware length in bytes
- digest: SHA-256 or board-approved digest over the firmware bytes
- signature: required detached signature or board-approved signed metadata

Metadata is validated before the image is accepted into the staging area.

## Interruption and recovery

Interrupted uploads are safe because the staging region is separated from the active sector. Any partial or failed upload remains isolated until verification succeeds. The session state transitions to ABORTED or FAILED and stops on a validation error.

## Host test backend

The repository includes a host verification backend for test automation. It is suitable for local validation only and does not imply production cryptographic security. It validates target and version metadata and allows an OTA session to progress in a test harness.

## package_ota.py

The packaging script creates metadata and a digest but does not generate or manage a private key. The developer must sign the image externally before deployment. The script rejects an unsafe assumption that private keys are present locally.

Example:

```bash
openssl dgst -sha256 -sign private-key.pem -out dist/firmware.bin.sig dist/firmware.bin
python3 tools/package_ota.py --image build/firmware.bin --target rp2354 --version 1.4.2 \
  --signature dist/firmware.bin.sig --out dist/
```

The packager refuses to emit an unsigned release package. Signing remains external,
and the board backend must cryptographically verify the signature against its
provisioned public trust root.

## Security limitations

- No production secure boot or cryptographic verification is claimed.
- No physical flash or bootloader validation was performed in this environment.
- The actual hardware, flash layout, and board boot flow remain unverified.
- The failure mode is intentionally conservative: fail closed unless an explicit board backend is configured.
