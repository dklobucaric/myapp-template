#!/usr/bin/env bash

set -euo pipefail

usage() {
  cat <<'USAGE'
Usage:
  ./scripts/build.sh [debug|release]

Examples:
  ./scripts/build.sh debug
  ./scripts/build.sh release
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
    printf 'Unknown build mode: %s\n\n' "$MODE" >&2
    usage >&2
    exit 1
    ;;
esac

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

cmake --preset "$PRESET"
cmake --build --preset "$PRESET" --parallel

printf '\nBuild complete:\n%s/build/%s/myapp-template\n' \
  "$ROOT_DIR" "$PRESET"
