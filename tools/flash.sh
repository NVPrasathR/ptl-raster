#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${1:-${ROOT_DIR}/build}"
IMAGE_PATH="${BUILD_DIR}/firmware/raster_pick_to_light.uf2"

if [[ ! -f "${IMAGE_PATH}" ]]; then
    echo "Firmware UF2 not found at ${IMAGE_PATH}. Run ./tools/build.sh first." >&2
    exit 1
fi

if ! command -v picotool >/dev/null 2>&1; then
    echo "picotool was not found in PATH." >&2
    exit 1
fi

picotool load -f "${IMAGE_PATH}"
picotool reboot
