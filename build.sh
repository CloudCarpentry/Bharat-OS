#!/usr/bin/env bash
# ----------------------------------------------------------------------------
# Bharat-OS user-facing wrapper script
# Real implementation lives in tools/build.py
# ----------------------------------------------------------------------------

set -e

if [[ "${1:-}" == "--sched-matrix" ]]; then
  shift
  root_dir="$(cd "$(dirname "$0")" && pwd)"
  declare -a archs=(x86_64 arm64 riscv64 arm32 riscv32)
  declare -a profiles=(gp rt mix)

  for arch in "${archs[@]}"; do
    for profile in "${profiles[@]}"; do
      bdir="build/${arch}-${profile}-headless"
      rm -rf "${root_dir}/${bdir}"
      defs=(
        -DARCH="${arch}"
        -DBHARAT_BOOT_GUI=OFF
        -DBHARAT_BOOT_HW_PROFILE=edge
        -DBHARAT_IRQ_DISPATCH_GENERIC=0
        -DBHARAT_IRQ_DISPATCH_RT=0
        -DBHARAT_IRQ_DISPATCH_MIXED=0
      )
      case "${profile}" in
        gp) defs+=(-DBHARAT_IRQ_DISPATCH_GENERIC=1) ;;
        rt) defs+=(-DBHARAT_IRQ_DISPATCH_RT=1) ;;
        mix) defs+=(-DBHARAT_IRQ_DISPATCH_MIXED=1) ;;
      esac

      cmake -S "${root_dir}" -B "${root_dir}/${bdir}" \
        -DCMAKE_TOOLCHAIN_FILE="${root_dir}/cmake/toolchains/${arch}-elf.cmake" \
        "${defs[@]}" "$@"
      cmake --build "${root_dir}/${bdir}" -j2 --target k_sched
    done
  done
  exit 0
fi

exec python3 tools/build.py "$@"
