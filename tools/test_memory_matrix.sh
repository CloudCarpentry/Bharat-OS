#!/usr/bin/env bash
# tools/test_memory_matrix.sh
# Automates testing of explicit memory capabilities using CMake Presets.

set -e

cd "$(dirname "$0")/.."

echo "========================================"
echo " Bharat-OS Memory Capability Harness    "
echo "========================================"

PRESETS=(
    "tiny-mpu-debug"
    "tiny-mpu-release"
    "edge-mmu-lite-debug"
    "gp-fullvm-debug"
    "gp-fullvm-release"
)

FAILS=0
PASSES=0

for preset in "${PRESETS[@]}"; do
    echo ""
    echo "----------------------------------------"
    echo " Testing Preset: $preset"
    echo "----------------------------------------"

    # Configure
    echo "[*] Configuring..."
    if cmake --preset "$preset"; then
        echo "[+] Configure successful."
    else
        echo "[-] Configure failed for $preset."
        FAILS=$((FAILS+1))
        continue
    fi

    # Build
    echo "[*] Building..."
    if cmake --build --preset "$preset"; then
        echo "[+] Build successful."
        PASSES=$((PASSES+1))
    else
        echo "[-] Build failed for $preset."
        FAILS=$((FAILS+1))
        continue
    fi
done

echo ""
echo "========================================"
echo " Harness Results"
echo "========================================"
echo " Passes: $PASSES"
echo " Fails:  $FAILS"

if [ "$FAILS" -gt 0 ]; then
    exit 1
else
    exit 0
fi
