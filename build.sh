#!/usr/bin/env bash

# Bharat-OS wrapper script
# Usage: ./build.sh <build_name> [options]
# This script is a thin wrapper over tools/build.py.

python3 tools/build.py "$@"
