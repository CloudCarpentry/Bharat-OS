#!/usr/bin/env bash
# tools/build.sh — Bharat-OS build + QEMU run script for Linux / macOS / WSL / BSD

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

SAF="\033[38;2;255;153;51m"
GRN="\033[38;2;19;136;8m"
RED="\033[0;31m"
RST="\033[0m"
inf() { echo -e "  ${SAF}[.]${RST} $1"; }
ok()  { echo -e "  ${GRN}[+]${RST} $1"; }

echo ""
echo -e "  ${SAF}Bharat-OS Build  (arch: ${ARCH})${RST}"
echo -e "  ${SAF}────────────────────────────────${RST}"
echo ""

case "${ARCH}" in
    x86_64)  TOOLCHAIN="cmake/toolchains/x86_64-elf.cmake" ;;
    riscv64) TOOLCHAIN="cmake/toolchains/riscv64-elf.cmake" ;;
    arm64)   TOOLCHAIN="cmake/toolchains/arm64-elf.cmake" ;;
esac

if [ "${CLEAN}" = "true" ] && [ -d "${BUILD_DIR}" ]; then
    inf "Cleaning ${BUILD_DIR}"
    rm -rf "${BUILD_DIR}"
fi

inf "Configuring (CMake)"
cmake -S "${BHARAT_ROOT}/kernel" \
      -B "${BUILD_DIR}" \
      -DCMAKE_TOOLCHAIN_FILE="${BHARAT_ROOT}/${TOOLCHAIN}" \
      -DBHARAT_BOOT_GUI="${BOOT_GUI}" \
      -DBHARAT_BOOT_HW_PROFILE="${BOOT_HW}" \
      -G Ninja \
      --no-warn-unused-cli

inf "Building kernel.elf"
cmake --build "${BUILD_DIR}" --target kernel.elf

SIZE=$(du -sh "${OUT_ELF}" 2>/dev/null | cut -f1)
ok "kernel.elf → ${OUT_ELF}  (${SIZE})"

if [ "${RUN}" = "true" ]; then
    echo ""
    ok "Booting in QEMU (press Ctrl+A then X to quit)..."
    echo ""
    case "${ARCH}" in
        x86_64)
            mkdir -p "${BUILD_DIR}/iso/boot/grub"
            cp "${OUT_ELF}" "${BUILD_DIR}/iso/boot/kernel.elf"
            cat << 'EOF2' > "${BUILD_DIR}/iso/boot/grub/grub.cfg"
set timeout=0
set default=0
menuentry "Bharat-OS" {
    multiboot2 /boot/kernel.elf
    boot
}
EOF2
            if command -v grub-mkrescue >/dev/null 2>&1; then
                grub-mkrescue -o "${BUILD_DIR}/bharat.iso" "${BUILD_DIR}/iso" 2>/dev/null || true
            fi

            if [ -f "${BUILD_DIR}/bharat.iso" ]; then
                qemu-system-x86_64 \
                    -cdrom "${BUILD_DIR}/bharat.iso" \
                    -m 256M \
                    -nographic \
                    -serial mon:stdio \
                    -no-reboot \
                    -boot d
            else
                qemu-system-x86_64 \
                    -kernel "${OUT_ELF}" \
                    -m 256M \
                    -nographic \
                    -serial mon:stdio \
                    -no-reboot
            fi
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
            qemu-system-aarch64 \
                -machine virt \
                -cpu cortex-a53 \
                -kernel "${OUT_ELF}" \
                -m 256M \
                -nographic \
                -serial mon:stdio \
                -no-reboot
            ;;
    esac
fi
