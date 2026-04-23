#!/usr/bin/env bash

# ----------------------------------------------------------------------------
# COMPATIBILITY SHIM
# Do not add new build/run logic here.
# Authoritative implementation is tools/build.py.
# Any new flag or run behavior must be implemented in tools/build.py only.
# ----------------------------------------------------------------------------

set -e

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
exec python3 "$DIR/build.py" "$@"
