#!/usr/bin/env bash
# ----------------------------------------------------------------------------
# Bharat-OS user-facing wrapper script
# Real implementation lives in tools/build.py
# ----------------------------------------------------------------------------

set -e
exec python3 tools/build.py "$@"
