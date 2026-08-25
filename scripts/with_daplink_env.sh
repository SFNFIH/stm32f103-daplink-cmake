#!/usr/bin/env bash
# Run a command with the DAPLink venv prepended to PATH.
set -euo pipefail

if [[ $# -lt 1 ]]; then
  echo "usage: $0 <command> [args...]" >&2
  exit 2
fi

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DAPLINK_ROOT="${DAPLINK_ROOT:-$ROOT/third_party/DAPLink}"
export PATH="${DAPLINK_ROOT}/venv/bin:${PATH}"

exec "$@"
