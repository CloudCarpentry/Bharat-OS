#!/usr/bin/env bash

# run_kernel_verification.sh
# Verifies the build artifact run manifest against the kernel verifier.

set -e

if [ $# -lt 1 ]; then
    echo "Usage: $0 <path_to_run_manifest.json>"
    exit 1
fi

MANIFEST_PATH="$1"

# Resolve tools path
TOOLS_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

echo "Running kernel verifier on: $MANIFEST_PATH"
python3 "$TOOLS_DIR/verify/kernel_verifier.py" "$MANIFEST_PATH"
