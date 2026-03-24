#!/usr/bin/env bash
# run_qemu_e2e.sh - End-to-end QEMU harness for multi-arch/profile Bharat-OS boots.

set -euo pipefail

TIMEOUT_SECS="${QEMU_TIMEOUT_SECS:-15}"
STRICT_QEMU="${STRICT_QEMU:-0}"

# Format: arch:profile:personality (profile/personality are passed to CMake as uppercase enums)
DEFAULT_MATRIX=(
  "x86_64:desktop:none"
  "x86_64:laptop:none"
  "riscv64:edge:none"
  "arm64:drone:none"
)

if [[ -n "${E2E_MATRIX:-}" ]]; then
  # shellcheck disable=SC2206
  MATRIX=( ${E2E_MATRIX} )
else
  MATRIX=("${DEFAULT_MATRIX[@]}")
fi

PASS_COUNT=0
FAIL_COUNT=0
SKIP_COUNT=0

mkdir -p e2e_logs

find_kernel_elf() {
  local build_dir="$1"
  local candidate="${build_dir}/kernel/kernel.elf"
  if [[ -f "$candidate" ]]; then
    printf '%s\n' "$candidate"
    return 0
  fi

  candidate="$(find "$build_dir" -maxdepth 4 -type f -name kernel.elf | head -n 1 || true)"
  if [[ -n "$candidate" ]]; then
    printf '%s\n' "$candidate"
    return 0
  fi

  return 1
}

objcopy_bin() {
  if command -v llvm-objcopy >/dev/null 2>&1; then
    printf 'llvm-objcopy\n'
  elif command -v objcopy >/dev/null 2>&1; then
    printf 'objcopy\n'
  else
    return 1
  fi
}

run_case() {
  local arch="$1"
  local profile="$2"
  local personality="$3"

  local profile_upper personality_upper
  profile_upper="$(printf '%s' "$profile" | tr '[:lower:]' '[:upper:]')"
  personality_upper="$(printf '%s' "$personality" | tr '[:lower:]' '[:upper:]')"

  local build_dir="build_e2e_${arch}_${profile}_${personality}"
  local log_file="e2e_logs/boot_${arch}_${profile}_${personality}.log"

  local qemu_bin machine_arg cpu_arg=()
  case "$arch" in
    x86_64)
      qemu_bin="qemu-system-x86_64"
      machine_arg=()
      ;;
    riscv64)
      qemu_bin="qemu-system-riscv64"
      machine_arg=(-machine virt)
      ;;
    arm64)
      qemu_bin="qemu-system-aarch64"
      machine_arg=(-machine virt)
      cpu_arg=(-cpu cortex-a72)
      ;;
    *)
      echo "[FAIL] Unsupported architecture in matrix: ${arch}"
      FAIL_COUNT=$((FAIL_COUNT + 1))
      return
      ;;
  esac

  echo "[*] Case: arch=${arch}, profile=${profile_upper}, personality=${personality_upper}"

  if ! command -v "$qemu_bin" >/dev/null 2>&1; then
    echo "    [SKIP] ${qemu_bin} not found in PATH"
    SKIP_COUNT=$((SKIP_COUNT + 1))
    return
  fi

  cmake -S . -B "$build_dir" \
    -DBHARAT_ARCH_FAMILY="${arch}" \
    -DBHARAT_DEVICE_PROFILE="${profile_upper}" \
    -DBHARAT_PERSONALITY_PROFILE="${personality_upper}" >/dev/null

  cmake --build "$build_dir" --target kernel.elf >/dev/null

  local kernel_elf
  if ! kernel_elf="$(find_kernel_elf "$build_dir")"; then
    echo "    [FAIL] kernel.elf not found in ${build_dir}"
    FAIL_COUNT=$((FAIL_COUNT + 1))
    return
  fi

  local kernel_image="$kernel_elf"
  if [[ "$arch" == "arm64" ]]; then
    local oc
    if ! oc="$(objcopy_bin)"; then
      echo "    [FAIL] objcopy/llvm-objcopy is required for arm64 Image generation"
      FAIL_COUNT=$((FAIL_COUNT + 1))
      return
    fi
    kernel_image="${build_dir}/kernel/Image"
    "$oc" -O binary "$kernel_elf" "$kernel_image"
  fi

  : > "$log_file"

  set +e
  timeout --preserve-status "$TIMEOUT_SECS" \
    "$qemu_bin" \
    "${machine_arg[@]}" \
    "${cpu_arg[@]}" \
    -kernel "$kernel_image" \
    -m 256M \
    -nographic \
    -monitor none \
    -serial stdio \
    -no-reboot >"$log_file" 2>&1
  local qemu_status=$?
  set -e

  if [[ "$qemu_status" -ne 0 && "$qemu_status" -ne 124 ]]; then
    echo "    [FAIL] QEMU exited unexpectedly with status ${qemu_status}"
    tail -n 80 "$log_file" || true
    FAIL_COUNT=$((FAIL_COUNT + 1))
    return
  fi

  local markers=(
    "BOOT: pmm initialized"
    "BOOT: vmm initialized"
    "[BOOT] Runtime mode:"
  )

  local missing=0
  for marker in "${markers[@]}"; do
    if ! grep -Fq "$marker" "$log_file"; then
      echo "    [FAIL] Missing marker: ${marker}"
      missing=1
    fi
  done

  if grep -Fq "[PANIC]" "$log_file"; then
    echo "    [FAIL] Panic detected in QEMU log"
    missing=1
  fi

  if [[ "$missing" -ne 0 ]]; then
    echo "    [INFO] Last 80 log lines (${log_file}):"
    tail -n 80 "$log_file" || true
    FAIL_COUNT=$((FAIL_COUNT + 1))
    return
  fi

  echo "    [PASS] Boot markers found"
  PASS_COUNT=$((PASS_COUNT + 1))
}

echo "======================================"
echo " Bharat-OS QEMU E2E Harness"
echo " Timeout per case: ${TIMEOUT_SECS}s"
echo "======================================"

for spec in "${MATRIX[@]}"; do
  IFS=':' read -r arch profile personality <<< "$spec"
  if [[ -z "${arch:-}" || -z "${profile:-}" || -z "${personality:-}" ]]; then
    echo "[FAIL] Invalid matrix entry: ${spec} (expected arch:profile:personality)"
    FAIL_COUNT=$((FAIL_COUNT + 1))
    continue
  fi
  run_case "$arch" "$profile" "$personality"
done

echo "======================================"
echo " Summary: PASS=${PASS_COUNT} FAIL=${FAIL_COUNT} SKIP=${SKIP_COUNT}"
echo " Logs: e2e_logs/"
echo "======================================"

if [[ "$FAIL_COUNT" -ne 0 ]]; then
  exit 1
fi

if [[ "$STRICT_QEMU" == "1" && "$SKIP_COUNT" -ne 0 ]]; then
  echo "STRICT_QEMU=1 set and some test cases were skipped"
  exit 1
fi

exit 0
