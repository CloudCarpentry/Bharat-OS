#!/bin/bash
set -e

TARGET="arm64_automotive_headless"
OUTPUT_LOG="automotive_smoke.log"

echo "Building automotive target..."
./build.sh all --target $TARGET

echo "Running automotive smoke test in QEMU..."
# Using direct qemu call because build.py run doesn't seem to support -append easily or is failing
# We get the command from build.py output but add our -append
QEMU_CMD="qemu-system-aarch64 -machine virt,gic-version=2 -cpu cortex-a57 -m 256M -kernel build/arm64-edge/kernel/kernel.elf -nographic -monitor none -serial stdio -no-reboot -smp 1 -append mode=automotive"

$QEMU_CMD > $OUTPUT_LOG 2>&1 &
QEMU_PID=$!

MAX_WAIT=30
COUNT=0
SUCCESS=0

while [ $COUNT -lt $MAX_WAIT ]; do
    if grep -q "AUTO_SMOKE: completed" $OUTPUT_LOG; then
        echo "Found completion marker!"
        SUCCESS=1
        break
    fi
    sleep 1
    COUNT=$((COUNT+1))
done

kill $QEMU_PID || true

if [ $SUCCESS -eq 1 ]; then
    echo "Automotive smoke test PASSED"
    python3 quality/e2e/qemu/check_auto_trace.py $OUTPUT_LOG
else
    echo "Automotive smoke test FAILED (timeout)"
    cat $OUTPUT_LOG
    [ $SUCCESS -eq 1 ]
fi
