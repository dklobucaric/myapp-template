#!/usr/bin/env bash

set -euo pipefail

usage() {
  cat <<'USAGE'
Usage:
  ./scripts/test.sh [debug|release]

Examples:
  ./scripts/test.sh
  ./scripts/test.sh debug
  ./scripts/test.sh release
USAGE
}

MODE="${1:-debug}"

case "$MODE" in
  debug)
    PRESET="linux-debug"
    ;;
  release)
    PRESET="linux-release"
    ;;
  -h|--help)
    usage
    exit 0
    ;;
  *)
    printf 'Unknown test mode: %s\n\n' "$MODE" >&2
    usage >&2
    exit 1
    ;;
esac

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

cmake --preset "$PRESET"
cmake --build --preset "$PRESET" --parallel
ctest --test-dir "$ROOT_DIR/build/$PRESET" --output-on-failure
