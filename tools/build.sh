#!/usr/bin/env bash
# tools/build.sh — Bharat-OS build + QEMU run script for Linux / macOS / WSL / BSD
#
# Usage:
#   ./tools/build.sh                                  # build x86_64 (default)
#   ./tools/build.sh riscv64                         # build RISC-V 64-bit
#   ./tools/build.sh arm64                           # build ARM64 (compile-only)
#   ./tools/build.sh x86_64 --run                    # build and boot in QEMU
#   ./tools/build.sh x86_64 --clean --run            # clean build + QEMU
#   ./tools/build.sh x86_64 --boot-gui=ON --hw=vm    # configure boot knobs

set -euo pipefail

ARCH="${1:-x86_64}"
SHIFT_DONE=false
if [ $# -ge 1 ]; then shift; SHIFT_DONE=true; fi

CLEAN=false
RUN=false
BOOT_GUI="ON"
BOOT_HW="generic"
for arg in "$@"; do
    case "$arg" in
        --clean) CLEAN=true ;;
        --run)   RUN=true   ;;
        --boot-gui=*) BOOT_GUI="${arg#*=}" ;;
        --hw=*) BOOT_HW="${arg#*=}" ;;
    esac
done

BHARAT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="${BHARAT_ROOT}/build/${ARCH}"
OUT_ELF="${BUILD_DIR}/kernel.elf"

# ── Colour helpers ───────────────────────────────────────────────────────────
SAF="\033[38;2;255;153;51m"
GRN="\033[38;2;19;136;8m"
RED="\033[0;31m"
RST="\033[0m"
inf() { echo -e "  ${SAF}[.]${RST} $1"; }
ok()  { echo -e "  ${GRN}[+]${RST} $1"; }
err() { echo -e "  ${RED}[!]${RST} $1"; exit 1; }

echo ""
echo -e "  ${SAF}Bharat-OS Build  (arch: ${ARCH})${RST}"
echo -e "  ${SAF}────────────────────────────────${RST}"
echo ""

# ── Select toolchain file ───────────────────────────────────────────────────
case "${ARCH}" in
    x86_64)  TOOLCHAIN="cmake/toolchains/x86_64-elf.cmake" ;;
    riscv64) TOOLCHAIN="cmake/toolchains/riscv64-elf.cmake" ;;
    arm64)   TOOLCHAIN="cmake/toolchains/arm64-elf.cmake" ;;
    *)        err "Unknown arch: ${ARCH}. Supported: x86_64, riscv64, arm64" ;;
esac

# ── Clean ────────────────────────────────────────────────────────────────────
if [ "${CLEAN}" = "true" ] && [ -d "${BUILD_DIR}" ]; then
    inf "Cleaning ${BUILD_DIR}"
    rm -rf "${BUILD_DIR}"
fi

# ── Configure ────────────────────────────────────────────────────────────────
if [ ! -f "${BUILD_DIR}/CMakeCache.txt" ]; then
    inf "Configuring (CMake)"
    cmake -S "${BHARAT_ROOT}/kernel" \
          -B "${BUILD_DIR}" \
          -DCMAKE_TOOLCHAIN_FILE="${BHARAT_ROOT}/${TOOLCHAIN}" \
          -DBHARAT_BOOT_GUI="${BOOT_GUI}" \
          -DBHARAT_BOOT_HW_PROFILE="${BOOT_HW}" \
          -G Ninja \
          --no-warn-unused-cli
fi

# ── Build ─────────────────────────────────────────────────────────────────────
inf "Building kernel.elf"
cmake --build "${BUILD_DIR}" --target kernel.elf

SIZE=$(du -sh "${OUT_ELF}" 2>/dev/null | cut -f1)
ok "kernel.elf → ${OUT_ELF}  (${SIZE})"

# ── QEMU ─────────────────────────────────────────────────────────────────────
if [ "${RUN}" = "true" ]; then
    echo ""
    ok "Booting in QEMU (press Ctrl+A then X to quit)..."
    echo ""
    case "${ARCH}" in
        x86_64)
            qemu-system-x86_64 \
                -kernel "${OUT_ELF}" \
                -m 256M \
                -nographic \
                -serial mon:stdio \
                -no-reboot
            ;;
        riscv64)
            qemu-system-riscv64 \
                -machine virt \
                -kernel "${OUT_ELF}" \
                -m 256M \
                -nographic \
                -serial mon:stdio \
                -no-reboot
            ;;
        arm64)
            err "QEMU run is not wired for arm64 yet; build completed successfully."
            ;;
    esac
fi
