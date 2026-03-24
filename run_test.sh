#!/bin/bash
./tools/build.sh -Arch riscv64 -Run --e2e &
PID=$!
sleep 30
kill $PID
