#!/usr/bin/env bash
set -eu
python3 "$(dirname "$0")/package_ota.py" "$@"
