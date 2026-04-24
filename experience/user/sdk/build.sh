#!/usr/bin/env bash
# SDK compatibility wrapper - strictly forwards
set -e
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../" && pwd)"
exec "$ROOT_DIR/build.sh" "$@"
