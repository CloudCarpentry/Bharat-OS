#!/bin/bash
./build.sh all --target riscv64_desktop_headless &
PID=$!
sleep 30
kill $PID
