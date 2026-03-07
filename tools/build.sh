#!/usr/bin/env bash
# tools/build.sh — Bharat-OS build + QEMU run script for Linux / macOS / WSL / BSD

set -euo pipefail

ARCH="${1:-x86_64}"
SHIFT_DONE=false
if [ $# -ge 1 ]; then shift; SHIFT_DONE=true; fi

CLEAN=false
RUN=false
DEBUG=false
PAYLOAD=false
BOOT_GUI="ON"
BOOT_HW="generic"
MACHINE="virt"
for arg in "$@"; do
    case "$arg" in
        --clean) CLEAN=true ;;
        --run)   RUN=true   ;;
        --debug) DEBUG=true ;;
        --payload) PAYLOAD=true ;;
        --boot-gui=*) BOOT_GUI="${arg#*=}" ;;
        --hw=*) BOOT_HW="${arg#*=}" ;;
        --machine=*) MACHINE="${arg#*=}" ;;
    esac
done

BHARAT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="${BHARAT_ROOT}/build/${ARCH}"

if [ "${PAYLOAD}" = "true" ] && [ "${ARCH}" = "riscv64" ]; then
    BUILD_DIR="${BHARAT_ROOT}/build/${ARCH}-gcc"
fi

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
    riscv64)
        if [ "${PAYLOAD}" = "true" ]; then
            TOOLCHAIN="cmake/toolchains/riscv64-elf-gcc.cmake"
        else
            TOOLCHAIN="cmake/toolchains/riscv64-elf.cmake"
        fi
        ;;
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
if [ "${PAYLOAD}" = "true" ] && [ "${ARCH}" = "riscv64" ]; then
    cmake --build "${BUILD_DIR}" --target kernel.payload.bin
    SIZE=$(du -sh "${BUILD_DIR}/payload.bin" 2>/dev/null | cut -f1)
    ok "payload.bin → ${BUILD_DIR}/payload.bin  (${SIZE})"
else
    cmake --build "${BUILD_DIR}" --target kernel.elf
    SIZE=$(du -sh "${OUT_ELF}" 2>/dev/null | cut -f1)
    ok "kernel.elf → ${OUT_ELF}  (${SIZE})"
fi

if [ "${RUN}" = "true" ]; then
    DEBUG_ARGS=()
    if [ "${DEBUG}" = "true" ]; then
        inf "GDB Server enabled on tcp::1234. Waiting for debugger..."
        DEBUG_ARGS=("-s" "-S")
    fi

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
                    -boot d \
                    "${DEBUG_ARGS[@]}"
            else
                qemu-system-x86_64 \
                    -kernel "${OUT_ELF}" \
                    -m 256M \
                    -nographic \
                    -serial mon:stdio \
                    -no-reboot \
                    "${DEBUG_ARGS[@]}"
            fi
            ;;
        riscv64)
            if [ "${PAYLOAD}" = "true" ]; then
                # Example: payload.bin run flow, requires fw_payload integration generally,
                # but we'll try to boot it as a bios/kernel depending on QEMU capability.
                # For generic virt, QEMU can take kernel or bios.
                qemu-system-riscv64 \
                    -machine "${MACHINE}" \
                    -bios "${BUILD_DIR}/payload.bin" \
                    -m 256M \
                    -nographic \
                    -serial mon:stdio \
                    -no-reboot \
                    "${DEBUG_ARGS[@]}"
            else
                qemu-system-riscv64 \
                    -machine "${MACHINE}" \
                    -kernel "${OUT_ELF}" \
                    -m 256M \
                    -nographic \
                    -serial mon:stdio \
                    -no-reboot \
                    "${DEBUG_ARGS[@]}"
            fi
            ;;
        arm64)
            qemu-system-aarch64 \
                -machine "${MACHINE}" \
                -cpu cortex-a53 \
                -kernel "${OUT_ELF}" \
                -m 256M \
                -nographic \
                -serial mon:stdio \
                -no-reboot \
                "${DEBUG_ARGS[@]}"
            ;;
    esac
fi
