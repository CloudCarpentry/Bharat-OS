#!/usr/bin/env bash
set -euo pipefail

if [ "$#" -lt 3 ]; then
  echo "usage: $0 <arch:x86_64|riscv64> <kernel.elf> <log-file> [expected-marker ...]" >&2
  exit 2
fi

ARCH="$1"
KERNEL="$2"
LOG_FILE="$3"
shift 3

TIMEOUT_SECS="${QEMU_TIMEOUT_SECS:-12}"

mkdir -p "$(dirname "$LOG_FILE")"
: > "$LOG_FILE"

case "$ARCH" in
  x86_64)
    QEMU_BIN=qemu-system-x86_64
    QEMU_ARGS=(
      -kernel "$KERNEL"
      -m 256M
      -no-reboot
      -nographic
      -monitor none
      -serial stdio
    )
    ;;
  riscv64)
    QEMU_BIN=qemu-system-riscv64
    QEMU_ARGS=(
      -machine virt
      -kernel "$KERNEL"
      -m 256M
      -no-reboot
      -nographic
      -monitor none
      -serial stdio
    )
    ;;
  *)
    echo "unsupported arch: $ARCH" >&2
    exit 2
    ;;
esac

set +e
timeout --preserve-status "$TIMEOUT_SECS" "$QEMU_BIN" "${QEMU_ARGS[@]}" >"$LOG_FILE" 2>&1
qemu_status=$?
set -e

# Timeout (124) is expected for a kernel that intentionally halts in a loop.
if [ "$qemu_status" -ne 0 ] && [ "$qemu_status" -ne 124 ]; then
  echo "QEMU exited with unexpected status: $qemu_status" >&2
  tail -n 200 "$LOG_FILE" >&2 || true
  exit "$qemu_status"
fi

for marker in "$@"; do
  if ! grep -Fq "$marker" "$LOG_FILE"; then
    echo "Missing expected marker: $marker" >&2
    tail -n 200 "$LOG_FILE" >&2 || true
    exit 1
  fi
done

echo "QEMU log assertions passed for $ARCH"
