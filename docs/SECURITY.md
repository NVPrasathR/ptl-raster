# Security Notes

## Threat model and assumptions

This repository intentionally does not claim verified production-grade security for the OTA path or the embedded bootloader. The implementation is a safe, fail-closed design that prevents accidental overwrites, but physical hardware validation is still required before deployment.

## Safe OTA design principles

- Authentication is performed via an integration callback instead of embedded static credentials.
- The active firmware image is never overwritten directly.
- The system requires a staging area and a separate health-confirm transition.
- Metadata validation fails closed when a target backend is not configured.
- The update state machine requires an expected SHA-256 digest and detached signature,
  then delegates cryptographic verification to the production board backend.
- The package step only generates metadata and a digest; it never creates or stores private keys.

## Explicit lack of verification

The following items are not validated in this repository because their actual hardware backend was not available:

- RP2354B flash layout and erase/program semantics
- Board bootloader sector mapping and fallback behavior
- Physical signature verification path in the production bootloader
- Secure key storage and public-key trust chain
- Actual W5500 or WS2812 hardware timing beyond the UI and OTA safety contract

## Files intentionally kept as a safe boundary

- `firmware/ota/ota_state.c` and `ota_state.h` implement the state machine and fail-closed transitions.
- `firmware/ota/host_verify_backend.c` is provided for local testing only.
- `tools/package_ota.py` does not manage private keys or sign images.

## Deployment requirement

Before production use, a board-specific backend must be installed and configured to validate the exact signed image format, flash slot layout, and health-confirm logic. Until that backend exists, the OTA system remains offline-safe but not field deployable.

All mutating HTTP routes fail closed unless the transport supplies an administrative
authorizer and a valid `Authorization` credential. Credentials must be provisioned
uniquely during controlled commissioning; there is no default production password.
The credential SHA-256 value belongs in the validated device configuration through
the board's local manufacturing/provisioning flow. Until that flow and persistent
flash backend are installed, administrative HTTP operations remain deliberately
unavailable.
