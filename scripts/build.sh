#!/usr/bin/env bash
# Configure + build STM32F103 DAPLink firmware.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-$ROOT/build}"
INTERFACE_PROJECT="${INTERFACE_PROJECT:-stm32f103xb_if}"

cmake -S "${ROOT}" -B "${BUILD_DIR}" \
  -DDAPLINK_INTERFACE_PROJECT="${INTERFACE_PROJECT}" \
  "$@"

cmake --build "${BUILD_DIR}" --target daplink-firmware

echo
echo "Firmware:"
ls -la "${BUILD_DIR}/firmware/"
