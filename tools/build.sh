#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SDK_PATH="${PICO_SDK_PATH:-/System/Volumes/Data/private/tmp/pico-sdk}"
BUILD_DIR="${1:-${ROOT_DIR}/build}"

if [[ ! -f "${SDK_PATH}/pico_sdk_init.cmake" ]]; then
    echo "Pico SDK not found at ${SDK_PATH}. Set PICO_SDK_PATH to the SDK directory." >&2
    exit 1
fi

cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" -G Ninja \
    -DPICO_SDK_PATH="${SDK_PATH}" \
    -DPICO_BOARD=raster_custom

cmake --build "${BUILD_DIR}"
