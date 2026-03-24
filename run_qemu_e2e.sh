#!/usr/bin/env bash
# run_qemu_e2e.sh - End-to-end QEMU harness for multi-arch/profile Bharat-OS boots.

set -euo pipefail

TIMEOUT_SECS="${QEMU_TIMEOUT_SECS:-15}"
STRICT_QEMU="${STRICT_QEMU:-0}"
MATRIX_MODE="${E2E_MATRIX_MODE:-auto}" # auto | explicit

# Matrix entry format: arch:memory_test:device_profile:personality
# memory_test: mmu | mmu-lite | mpu | iommu
#
# Default auto matrix purposefully includes all requested HW arches and memory tests
# while pruning invalid architecture/profile combinations.
E2E_ARCHES=(x86_64 arm32 arm64 riscv32 riscv64)
E2E_MEMORY_TESTS=(mmu mmu-lite mpu iommu)
E2E_DEVICE_PROFILES=(desktop edge drone)
E2E_PERSONALITIES=(none linux)

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

supports_combo() {
  local arch="$1"
  local memory_test="$2"

  case "$arch:$memory_test" in
    x86_64:mmu|x86_64:iommu) return 0 ;;
    arm64:mmu|arm64:mmu-lite|arm64:iommu) return 0 ;;
    riscv64:mmu|riscv64:mmu-lite|riscv64:iommu) return 0 ;;
    arm32:mmu-lite|arm32:mpu) return 0 ;;
    riscv32:mmu-lite|riscv32:mpu) return 0 ;;
    *) return 1 ;;
  esac
}

set_memory_cmake_flags() {
  local memory_test="$1"

  case "$memory_test" in
    mmu|iommu)
      printf '%s\n' "-DBHARAT_PROFILE_MMU_FULL=ON -DBHARAT_PROFILE_MMU_LITE=OFF -DBHARAT_PROFILE_MPU_ONLY=OFF"
      ;;
    mmu-lite)
      printf '%s\n' "-DBHARAT_PROFILE_MMU_FULL=OFF -DBHARAT_PROFILE_MMU_LITE=ON -DBHARAT_PROFILE_MPU_ONLY=OFF"
      ;;
    mpu)
      printf '%s\n' "-DBHARAT_PROFILE_MMU_FULL=OFF -DBHARAT_PROFILE_MMU_LITE=OFF -DBHARAT_PROFILE_MPU_ONLY=ON"
      ;;
    *)
      return 1
      ;;
  esac
}

get_qemu_config() {
  local arch="$1"

  case "$arch" in
    x86_64)
      printf '%s\n' "qemu-system-x86_64"
      printf '\n'
      printf '\n'
      return 0
      ;;
    riscv64)
      printf '%s\n' "qemu-system-riscv64"
      printf '%s\n' "-machine virt"
      printf '\n'
      return 0
      ;;
    arm64)
      printf '%s\n' "qemu-system-aarch64"
      printf '%s\n' "-machine virt"
      printf '%s\n' "-cpu cortex-a72"
      return 0
      ;;
    *)
      # arm32/riscv32 currently don't have a validated QEMU boot command in this harness.
      return 1
      ;;
  esac
}

run_case() {
  local arch="$1"
  local memory_test="$2"
  local profile="$3"
  local personality="$4"

  if ! supports_combo "$arch" "$memory_test"; then
    echo "[SKIP] Invalid combo: arch=${arch}, memory_test=${memory_test}"
    SKIP_COUNT=$((SKIP_COUNT + 1))
    return
  fi

  local profile_upper personality_upper
  profile_upper="$(printf '%s' "$profile" | tr '[:lower:]' '[:upper:]')"
  personality_upper="$(printf '%s' "$personality" | tr '[:lower:]' '[:upper:]')"

  local build_dir="build_e2e_${arch}_${memory_test}_${profile}_${personality}"
  local log_file="e2e_logs/boot_${arch}_${memory_test}_${profile}_${personality}.log"

  echo "[*] Case: arch=${arch}, memory_test=${memory_test}, profile=${profile_upper}, personality=${personality_upper}"

  local flags
  if ! flags="$(set_memory_cmake_flags "$memory_test")"; then
    echo "    [FAIL] Unsupported memory_test value: ${memory_test}"
    FAIL_COUNT=$((FAIL_COUNT + 1))
    return
  fi

  # shellcheck disable=SC2206
  local cmake_flags=( $flags )

  cmake -S . -B "$build_dir" \
    -DBHARAT_ARCH_FAMILY="${arch}" \
    -DBHARAT_DEVICE_PROFILE="${profile_upper}" \
    -DBHARAT_PERSONALITY_PROFILE="${personality_upper}" \
    "${cmake_flags[@]}" >/dev/null

  cmake --build "$build_dir" --target kernel.elf >/dev/null

  local kernel_elf
  if ! kernel_elf="$(find_kernel_elf "$build_dir")"; then
    echo "    [FAIL] kernel.elf not found in ${build_dir}"
    FAIL_COUNT=$((FAIL_COUNT + 1))
    return
  fi

  local qemu_bin machine_line cpu_line
  if ! qemu_cfg="$(get_qemu_config "$arch")"; then
    echo "    [SKIP] Build-only arch in this harness (no validated QEMU boot command): ${arch}"
    SKIP_COUNT=$((SKIP_COUNT + 1))
    return
  fi

  qemu_bin="$(printf '%s\n' "$qemu_cfg" | sed -n '1p')"
  machine_line="$(printf '%s\n' "$qemu_cfg" | sed -n '2p')"
  cpu_line="$(printf '%s\n' "$qemu_cfg" | sed -n '3p')"

  if ! command -v "$qemu_bin" >/dev/null 2>&1; then
    echo "    [SKIP] ${qemu_bin} not found in PATH"
    SKIP_COUNT=$((SKIP_COUNT + 1))
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

  local qemu_args=(-kernel "$kernel_image" -m 256M -nographic -monitor none -serial stdio -no-reboot)
  if [[ -n "$machine_line" ]]; then
    # shellcheck disable=SC2206
    qemu_args=( $machine_line "${qemu_args[@]}" )
  fi
  if [[ -n "$cpu_line" ]]; then
    # shellcheck disable=SC2206
    qemu_args=( $cpu_line "${qemu_args[@]}" )
  fi

  set +e
  timeout --preserve-status "$TIMEOUT_SECS" \
    "$qemu_bin" \
    "${qemu_args[@]}" >"$log_file" 2>&1
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
    "[SELFTEST] Stage="
  )

  if [[ "$memory_test" == "iommu" ]]; then
    markers+=("IOMMU:")
  fi

  local missing=0
  for marker in "${markers[@]}"; do
    if ! grep -Fq "$marker" "$log_file"; then
      echo "    [FAIL] Missing marker: ${marker}"
      missing=1
    fi
  done

  if grep -Fq "[PANIC]" "$log_file" || grep -Fq "Fatal boot self-test failure" "$log_file"; then
    echo "    [FAIL] Panic/self-test fatal failure detected in QEMU log"
    missing=1
  fi

  if [[ "$missing" -ne 0 ]]; then
    echo "    [INFO] Last 80 log lines (${log_file}):"
    tail -n 80 "$log_file" || true
    FAIL_COUNT=$((FAIL_COUNT + 1))
    return
  fi

  echo "    [PASS] Boot + selftest markers found"
  PASS_COUNT=$((PASS_COUNT + 1))
}

echo "======================================"
echo " Bharat-OS QEMU E2E Harness"
echo " Timeout per case: ${TIMEOUT_SECS}s"
echo " Matrix mode: ${MATRIX_MODE}"
echo "======================================"

if [[ "$MATRIX_MODE" == "explicit" ]]; then
  if [[ -z "${E2E_MATRIX:-}" ]]; then
    echo "[FAIL] E2E_MATRIX_MODE=explicit but E2E_MATRIX is empty"
    exit 1
  fi

  # shellcheck disable=SC2206
  MATRIX=( ${E2E_MATRIX} )
  for spec in "${MATRIX[@]}"; do
    IFS=':' read -r arch memory_test profile personality <<< "$spec"
    if [[ -z "${arch:-}" || -z "${memory_test:-}" || -z "${profile:-}" || -z "${personality:-}" ]]; then
      echo "[FAIL] Invalid matrix entry: ${spec} (expected arch:memory_test:profile:personality)"
      FAIL_COUNT=$((FAIL_COUNT + 1))
      continue
    fi
    run_case "$arch" "$memory_test" "$profile" "$personality"
  done
else
  for arch in "${E2E_ARCHES[@]}"; do
    for memory_test in "${E2E_MEMORY_TESTS[@]}"; do
      for profile in "${E2E_DEVICE_PROFILES[@]}"; do
        for personality in "${E2E_PERSONALITIES[@]}"; do
          run_case "$arch" "$memory_test" "$profile" "$personality"
        done
      done
    done
  done
fi

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
