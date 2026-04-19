#!/usr/bin/env bash
# ----------------------------------------------------------------------------
# Bharat-OS user-facing wrapper script
# Real implementation lives in tools/build.py
# ----------------------------------------------------------------------------

set -e

if [[ "${1:-}" == "--sched-matrix" ]]; then
  shift
  root_dir="$(cd "$(dirname "$0")" && pwd)"
  declare -a targets=(
    x86_64_desktop_headless
    x86_64_desktop_headless_rt
    x86_64_desktop_headless_mix
    arm64_desktop_headless
    arm64_desktop_headless_rt
    arm64_desktop_headless_mix
    riscv64_desktop_headless
    riscv64_desktop_headless_rt
    riscv64_desktop_headless_mix
    arm32_mmu_lite_headless_gp
    arm32_mmu_lite_headless_rt
    arm32_mmu_lite_headless_mix
    riscv32_mmu_lite_headless_gp
    riscv32_mmu_lite_headless_rt
    riscv32_mmu_lite_headless_mix
  )

  for target_name in "${targets[@]}"; do
    python3 "${root_dir}/tools/build.py" build --target "${target_name}" "$@"
  done
  exit 0
fi

exec python3 tools/build.py "$@"
