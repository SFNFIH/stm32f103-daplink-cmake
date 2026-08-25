#!/usr/bin/env bash
# Create / refresh the DAPLink Python virtualenv used by progen.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DAPLINK_ROOT="${DAPLINK_ROOT:-$ROOT/third_party/DAPLink}"
VENV="${DAPLINK_ROOT}/venv"

if [[ ! -f "${DAPLINK_ROOT}/requirements.txt" ]]; then
  echo "DAPLink not found at ${DAPLINK_ROOT}" >&2
  exit 1
fi

python3 -m venv "${VENV}"
# progen still imports pkg_resources (removed from setuptools>=81)
"${VENV}/bin/pip" install -U "pip" "setuptools>=70,<81" "wheel"
"${VENV}/bin/pip" install -r "${DAPLINK_ROOT}/requirements.txt"

echo "venv ready: ${VENV}"
echo "progen: $("${VENV}/bin/progen" --help >/dev/null && echo OK)"
