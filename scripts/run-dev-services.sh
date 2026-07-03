#!/usr/bin/env bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CDN_PORT=8088
LICENSE_PORT=8089

cleanup() {
  printf '\nStopping mock services...\n'

  if [[ -n "${CDN_PID:-}" ]] && kill -0 "$CDN_PID" 2>/dev/null; then
    kill "$CDN_PID"
  fi

  if [[ -n "${LICENSE_PID:-}" ]] && kill -0 "$LICENSE_PID" 2>/dev/null; then
    kill "$LICENSE_PID"
  fi
}

trap cleanup EXIT INT TERM

python3 -m http.server "$CDN_PORT" \
  --directory "$ROOT_DIR/dev/mock-cdn" \
  > "$ROOT_DIR/.mock-cdn.log" 2>&1 &
CDN_PID=$!

python3 -m http.server "$LICENSE_PORT" \
  --directory "$ROOT_DIR/dev/mock-license-server" \
  > "$ROOT_DIR/.mock-license-server.log" 2>&1 &
LICENSE_PID=$!

printf 'Mock CDN:            http://localhost:%s\n' "$CDN_PORT"
printf 'Mock license server: http://localhost:%s\n' "$LICENSE_PORT"
printf '\nPress Ctrl+C to stop both services.\n\n'

wait "$CDN_PID" "$LICENSE_PID"
