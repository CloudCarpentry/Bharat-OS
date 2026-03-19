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

if [[ "$2" == "--run" ]] || [[ "$1" == "--run" ]]; then
    RUN_FLAG=1
    if [[ "$1" == "--run" ]]; then
        BUILD_NAME="default_dev"
    fi
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

# Configure
echo "Configuring: $PRESET (Arch: $ARCH, Profile: $PROFILE, Personality: $PERSONALITY, Board: $BOARD)"
cmake --preset $PRESET \
    -DBHARAT_ARCH_FAMILY=$ARCH \
    -DBHARAT_DEVICE_PROFILE=$PROFILE \
    -DBHARAT_PERSONALITY_PROFILE=$PERSONALITY \
    -DBHARAT_BOOT_GUI=$GUI_FLAG

# Build
echo "Building..."
cmake --build --preset $PRESET

if [ "$RUN" = "true" ] || [ "$RUN_FLAG" -eq 1 ]; then
    echo "Running..."
    if [ "$ARCH" = "x86_64" ]; then
        qemu-system-x86_64 -kernel build/$PRESET/kernel.elf -m 512 -serial stdio -display none
    elif [ "$ARCH" = "arm64" ]; then
        qemu-system-aarch64 -M virt -cpu cortex-a53 -m 512 -kernel build/$PRESET/kernel.elf -serial stdio -display none
    elif [ "$ARCH" = "riscv64" ]; then
        qemu-system-riscv64 -M virt -m 512 -kernel build/$PRESET/kernel.elf -serial stdio -display none
    fi
fi
