#!/usr/bin/env bash

# Bharat-OS wrapper script
# Usage: ./build.sh <build_name_from_yaml> [--run]

if ! command -v yq &> /dev/null; then
    echo "yq (yaml parser) could not be found. Please install it to parse build_config.yml"
    echo "Fallback: running 'cmake --preset linux-x86_64-dev-debug'"
    cmake --preset linux-x86_64-dev-debug
    cmake --build --preset linux-x86_64-dev-debug
    exit 0
fi

BUILD_NAME=${1:-"default_dev"}
RUN_FLAG=0
DUAL_SERIAL_FLAG=0

for arg in "$@"; do
    if [[ "$arg" == "--run" ]]; then
        RUN_FLAG=1
    elif [[ "$arg" == "--dual-serial" ]]; then
        DUAL_SERIAL_FLAG=1
    fi
done

if [[ "$1" == "--run" ]] || [[ "$1" == "--dual-serial" ]]; then
    BUILD_NAME="default_dev"
fi

if ! yq eval ".builds.$BUILD_NAME" build_config.yml &> /dev/null; then
    echo "Build config '$BUILD_NAME' not found in build_config.yml"
    exit 1
fi

PRESET=$(yq eval ".builds.$BUILD_NAME.preset" build_config.yml)
ARCH=$(yq eval ".builds.$BUILD_NAME.arch" build_config.yml)
PROFILE=$(yq eval ".builds.$BUILD_NAME.profile" build_config.yml)
PERSONALITY=$(yq eval ".builds.$BUILD_NAME.personality" build_config.yml)
BOARD=$(yq eval ".builds.$BUILD_NAME.board" build_config.yml)
GUI=$(yq eval ".builds.$BUILD_NAME.gui" build_config.yml)
RUN=$(yq eval ".builds.$BUILD_NAME.run" build_config.yml)

GUI_FLAG="OFF"
if [ "$GUI" = "true" ]; then GUI_FLAG="ON"; fi

normalize_csv_value() {
    local raw="$1"
    local first
    IFS=',' read -r first _ <<< "$raw"
    first="${first#"${first%%[![:space:]]*}"}"
    first="${first%"${first##*[![:space:]]}"}"
    echo "$first"
}

PROFILE_CANONICAL=$(normalize_csv_value "$PROFILE")
PROFILE_CANONICAL="${PROFILE_CANONICAL^^}"

PERSONALITY_CANONICAL=$(normalize_csv_value "$PERSONALITY")
PERSONALITY_CANONICAL="${PERSONALITY_CANONICAL^^}"

BOARD_CANONICAL=$(normalize_csv_value "$BOARD")

# Configure
echo "Configuring: $PRESET (Arch: $ARCH, Profile: $PROFILE_CANONICAL, Personality: $PERSONALITY_CANONICAL, Board: $BOARD_CANONICAL)"
cmake --preset $PRESET \
    -DBHARAT_ARCH_FAMILY=$ARCH \
    -DBHARAT_DEVICE_PROFILE=$PROFILE_CANONICAL \
    -DBHARAT_PERSONALITY_PROFILE=$PERSONALITY_CANONICAL \
    -DBHARAT_TARGET_BOARD=$BOARD_CANONICAL \
    -DBHARAT_BOOT_GUI=$GUI_FLAG

# Build
echo "Building..."
cmake --build --preset $PRESET

if [ "$RUN" = "true" ] || [ "$RUN_FLAG" -eq 1 ]; then
    echo "Running..."

    QEMU_OPTS=""
    if [ "$GUI_FLAG" = "ON" ]; then
        if [ "$DUAL_SERIAL_FLAG" -eq 1 ]; then
            SERIAL_OPTS="-serial stdio -serial vc"
        else
            SERIAL_OPTS="-serial vc"
        fi

        if [ "$ARCH" = "x86_64" ]; then
            QEMU_OPTS="$SERIAL_OPTS -vga std"
        elif [ "$ARCH" = "arm64" ]; then
            QEMU_OPTS="$SERIAL_OPTS -device virtio-gpu-pci"
        elif [ "$ARCH" = "riscv64" ]; then
            QEMU_OPTS="$SERIAL_OPTS -device virtio-gpu-device -device ramfb"
        fi
    else
        QEMU_OPTS="-serial stdio -display none"
    fi

    if [ "$ARCH" = "x86_64" ]; then
        qemu-system-x86_64 -kernel build/$PRESET/kernel.elf -m 512 $QEMU_OPTS
    elif [ "$ARCH" = "arm64" ]; then
        qemu-system-aarch64 -M virt -cpu cortex-a53 -m 512 -kernel build/$PRESET/kernel.elf $QEMU_OPTS
    elif [ "$ARCH" = "arm32" ] && [ "$BOARD" = "avh-corstone310" ]; then
        VHT_Corstone_SSE-310 -a build/$PRESET/kernel.elf $QEMU_OPTS
    elif [ "$ARCH" = "riscv64" ]; then
        qemu-system-riscv64 -M virt -m 512 -kernel build/$PRESET/kernel.elf $QEMU_OPTS
    fi
fi
