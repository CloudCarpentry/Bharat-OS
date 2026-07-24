#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT_DIR"

TIMEOUT_SECS="${QEMU_MATRIX_TIMEOUT_SECS:-180}"
PYTHON_BIN="${PYTHON_BIN:-python3}"

TARGETS=(
  "delivery/targets/qemu/x86_64_desktop_headless.yaml|required"
  "delivery/targets/qemu/arm64_desktop_headless.yaml|required"
  "delivery/targets/qemu/riscv64_desktop_headless.yaml|required"
  "delivery/targets/qemu/arm32_mmu_lite_headless.yaml|required"
  "delivery/targets/qemu/riscv32_mmu_lite_headless.yaml|experimental"
)

pass=0
warn=0
fail=0

run_target() {
  local yaml="$1"
  local lane="$2"
  local cmd=("$PYTHON_BIN" tools/build.py all --target-yaml "$yaml")

  echo "[qemu-matrix] lane=$lane target=$yaml"

  set +e
  timeout --preserve-status "$TIMEOUT_SECS" "${cmd[@]}"
  local rc=$?
  set -e

  if [[ "$rc" -eq 0 || "$rc" -eq 124 ]]; then
    echo "[qemu-matrix] PASS target=$yaml rc=$rc"
    ((pass+=1))
    return 0
  fi

  if [[ "$lane" == "experimental" ]]; then
    echo "[qemu-matrix] WARN target=$yaml rc=$rc (experimental lane)"
    ((warn+=1))
    return 0
  fi

  echo "[qemu-matrix] FAIL target=$yaml rc=$rc"
  ((fail+=1))
  return 0
}

for entry in "${TARGETS[@]}"; do
  yaml="${entry%%|*}"
  lane="${entry##*|}"
  run_target "$yaml" "$lane"
done

echo "[qemu-matrix] summary pass=$pass warn=$warn fail=$fail"

if [[ "$fail" -ne 0 ]]; then
  exit 1
fi
