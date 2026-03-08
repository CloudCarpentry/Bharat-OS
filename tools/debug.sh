#!/usr/bin/env bash
ARCH=${1:-x86_64}

GDB="gdb-multiarch"
if ! command -v $GDB &> /dev/null; then
    GDB="gdb"
fi

echo "Starting GDB targeting localhost:1234 for $ARCH..."

$GDB -ex "target remote localhost:1234" \
     -ex "layout src" \
     -ex "break kernel_main" \
     -ex "continue"
