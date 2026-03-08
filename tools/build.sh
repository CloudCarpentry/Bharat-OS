#!/usr/bin/env bash
# tools/build.sh — Bharat-OS build + QEMU run script for Linux / macOS / WSL / BSD

set -euo pipefail

# Parse arguments
ARCH=""
BOARD=""
TOOLCHAIN_OVERRIDE=""
CLEAN=false
RUN=false
DEBUG=false
PAYLOAD=false
FLASH=false
BOOT_GUI="ON"
BOOT_HW=""
MACHINE=""
BOOT_TIER=""

# If first arg doesn't start with '-', consider it ARCH
if [ $# -ge 1 ] && [[ "$1" != -* ]]; then
    ARCH="$1"
    shift
fi

for arg in "$@"; do
    case "$arg" in
        --arch=*) ARCH="${arg#*=}" ;;
        --board=*) BOARD="${arg#*=}" ;;
        --toolchain=*) TOOLCHAIN_OVERRIDE="${arg#*=}" ;;
        --clean) CLEAN=true ;;
        --run)   RUN=true   ;;
        --debug) DEBUG=true ;;
        --payload) PAYLOAD=true ;;
        --flash) FLASH=true ;;
        --boot-gui=*) BOOT_GUI="${arg#*=}" ;;
        --hw=*) BOOT_HW="${arg#*=}" ;;
        --machine=*) MACHINE="${arg#*=}" ;;
        --tier=*) BOOT_TIER="${arg#*=}" ;;
    esac
done

BHARAT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"

# Parse boards.json if BOARD is specified
BOARDS_JSON="${BHARAT_ROOT}/tools/boards/boards.json"
BOARD_ARCH=""
BOARD_MACHINE=""
BOARD_QEMU_SYSTEM=""
BOARD_DEFAULT_TOOLCHAIN=""
BOARD_TOOLCHAIN_FILE=""
BOARD_HW_PROFILE=""
BOARD_TIER=""
BOARD_BOOT_HW=""
BOARD_FLASH_DIR=""

if [ -n "$BOARD" ]; then
    if [ ! -f "$BOARDS_JSON" ]; then
        echo "Error: boards.json not found at $BOARDS_JSON"
        exit 1
    fi
    # Use python to extract basic json info since jq might not be available
    BOARD_INFO=$(python3 -c "
import sys, json
try:
    with open('$BOARDS_JSON', 'r') as f:
        data = json.load(f)
    if '$BOARD' not in data.get('boards', {}):
        sys.exit(1)
    b = data['boards']['$BOARD']
    toolchains = b.get('toolchains', {})
    default_tc = b.get('default_toolchain', '')
    tc_file = toolchains.get(default_tc, '')
    print(f\"{b.get('arch', '')}|{b.get('machine', '')}|{b.get('qemu_system', '')}|{default_tc}|{tc_file}|{b.get('hardware_profile', '')}|{b.get('tier', '')}|{b.get('boot_hw', '')}|{b.get('flash_script_dir', '')}\")
except Exception as e:
    sys.exit(1)
" 2>/dev/null)

    if [ $? -ne 0 ] || [ -z "$BOARD_INFO" ]; then
        echo "Error: Board '$BOARD' not found in boards.json or invalid JSON"
        exit 1
    fi

    IFS='|' read -r BOARD_ARCH BOARD_MACHINE BOARD_QEMU_SYSTEM BOARD_DEFAULT_TOOLCHAIN BOARD_TOOLCHAIN_FILE BOARD_HW_PROFILE BOARD_TIER BOARD_BOOT_HW BOARD_FLASH_DIR <<< "$BOARD_INFO"

    # Validation/Conflict resolution
    if [ -n "$ARCH" ] && [ "$ARCH" != "$BOARD_ARCH" ]; then
        echo "Error: Board '$BOARD' requires arch '$BOARD_ARCH', but arch '$ARCH' was provided."
        exit 1
    fi

    # Derive from board
    ARCH="${BOARD_ARCH}"
    [ -z "$MACHINE" ] && MACHINE="${BOARD_MACHINE}"
    [ -z "$BOOT_HW" ] && BOOT_HW="${BOARD_HW_PROFILE}"
    [ -z "$BOOT_TIER" ] && BOOT_TIER="${BOARD_TIER}"
    # NOTE: We map BOOT_HW profile. The board boot_hw field corresponds to the machine/qemu boot hardware setup if needed.
fi

# Fallback defaults
[ -z "$ARCH" ] && ARCH="x86_64"
[ -z "$MACHINE" ] && MACHINE="virt"
[ -z "$BOOT_HW" ] && BOOT_HW="generic"
[ -z "$BOOT_TIER" ] && BOOT_TIER="LINUX_LIKE"
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

if [ -n "$TOOLCHAIN_OVERRIDE" ]; then
    # Could be a full path or a toolchain name from the board preset
    if [ -f "${BHARAT_ROOT}/cmake/toolchains/${ARCH}-${TOOLCHAIN_OVERRIDE}.cmake" ]; then
        TOOLCHAIN="cmake/toolchains/${ARCH}-${TOOLCHAIN_OVERRIDE}.cmake"
    elif [ -f "${BHARAT_ROOT}/${TOOLCHAIN_OVERRIDE}" ]; then
        TOOLCHAIN="${TOOLCHAIN_OVERRIDE}"
    elif [ -n "$BOARD" ]; then
        # Try to resolve toolchain name from boards.json
        BOARD_TC=$(python3 -c "
import sys, json
try:
    with open('$BOARDS_JSON', 'r') as f:
        data = json.load(f)
    print(data['boards']['$BOARD'].get('toolchains', {}).get('$TOOLCHAIN_OVERRIDE', ''))
except Exception as e:
    sys.exit(1)
" 2>/dev/null)
        if [ -n "$BOARD_TC" ]; then
            TOOLCHAIN="${BOARD_TC}"
        else
            echo "Error: Toolchain override '$TOOLCHAIN_OVERRIDE' not found."
            exit 1
        fi
    else
        echo "Error: Toolchain override '$TOOLCHAIN_OVERRIDE' not found."
        exit 1
    fi
elif [ -n "$BOARD" ] && [ -n "$BOARD_TOOLCHAIN_FILE" ]; then
    TOOLCHAIN="${BOARD_TOOLCHAIN_FILE}"
else
    # Fallback default generic mapping
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
fi

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
      -DBHARAT_BOOT_TIER="${BOOT_TIER}" \
      -G Ninja \
      --no-warn-unused-cli

inf "Building kernel.elf"
if [ "${PAYLOAD}" = "true" ] && [ "${ARCH}" = "riscv64" ]; then
    cmake --build "${BUILD_DIR}" --target kernel.payload.bin
    SIZE=$(du -sh "${BUILD_DIR}/payload.bin" 2>/dev/null | cut -f1)
    ok "payload.bin → ${BUILD_DIR}/payload.bin  (${SIZE})"

    if [ -n "${OPENSBI_DIR:-}" ] && [ -d "${OPENSBI_DIR}" ]; then
        inf "Building OpenSBI fw_payload.elf"
        make -C "${OPENSBI_DIR}" \
            PLATFORM=generic \
            CROSS_COMPILE=riscv64-unknown-elf- \
            FW_PAYLOAD_PATH="${BUILD_DIR}/payload.bin" \
            O="${BUILD_DIR}/opensbi" \
            > /dev/null
        cp "${BUILD_DIR}/opensbi/platform/generic/firmware/fw_payload.elf" "${BUILD_DIR}/fw_payload.elf"
        SIZE=$(du -sh "${BUILD_DIR}/fw_payload.elf" 2>/dev/null | cut -f1)
        ok "fw_payload.elf → ${BUILD_DIR}/fw_payload.elf  (${SIZE})"
    fi
else
    cmake --build "${BUILD_DIR}" --target kernel.elf
    SIZE=$(du -sh "${OUT_ELF}" 2>/dev/null | cut -f1)
    ok "kernel.elf → ${OUT_ELF}  (${SIZE})"
fi

if [ "${FLASH}" = "true" ]; then
    if [ -n "$BOARD_FLASH_DIR" ] && [ -f "${BHARAT_ROOT}/${BOARD_FLASH_DIR}/flash.sh" ]; then
        inf "Flashing using script: ${BHARAT_ROOT}/${BOARD_FLASH_DIR}/flash.sh"
        bash "${BHARAT_ROOT}/${BOARD_FLASH_DIR}/flash.sh" "${OUT_ELF}"
    else
        echo "Error: Flash requested, but no valid flash script found for board '$BOARD'."
        exit 1
    fi
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
                if [ -f "${BUILD_DIR}/fw_payload.elf" ]; then
                    qemu-system-riscv64 \
                        -machine "${MACHINE}" \
                        -bios none \
                        -kernel "${BUILD_DIR}/fw_payload.elf" \
                        -m 256M \
                        -nographic \
                        -serial mon:stdio \
                        -no-reboot \
                        "${DEBUG_ARGS[@]}"
                else
                    qemu-system-riscv64 \
                        -machine "${MACHINE}" \
                        -bios "${BUILD_DIR}/payload.bin" \
                        -m 256M \
                        -nographic \
                        -serial mon:stdio \
                        -no-reboot \
                        "${DEBUG_ARGS[@]}"
                fi
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
