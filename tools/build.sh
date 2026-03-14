#!/usr/bin/env bash

set -e

# Defaults
ARCH="x86_64"
BOARD=""
TOOLCHAIN_OVERRIDE=""
CLEAN=false
RUN=false
DEBUG=false
PAYLOAD=false
FLASH=false
BOOT_GUI=""
BOOT_HW="generic"
MACHINE=""
BOOT_TIER=""
PROFILE="desktop"
PERSONALITY="none"

# Parse arguments
for arg in "$@"; do
    case $arg in
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
        --profile=*) PROFILE="${arg#*=}" ;;
        --personality=*) PERSONALITY="${arg#*=}" ;;
        *)
            # If no equal sign, assume first argument is ARCH for backwards compatibility
            if [[ ! "$arg" == --* ]] && [[ -z "$ARCH_SET" ]]; then
                 ARCH="$arg"
                 ARCH_SET=true
            fi
            ;;
    esac
done

PROFILE_CLEAN="${PROFILE//,/-}"
PERSONALITY_CLEAN="${PERSONALITY//,/-}"

BUILD_DIR="build/${ARCH}_${PROFILE_CLEAN}_${BOOT_HW}_${PERSONALITY_CLEAN}"
if [ -n "$BOARD" ]; then
    BUILD_DIR="${BUILD_DIR}_${BOARD}"
fi

if [ "$CLEAN" = true ]; then
    echo "Cleaning build directory: ${BUILD_DIR}"
    rm -rf "${BUILD_DIR}"
fi

CMAKE_ARGS=""

IFS=',' read -ra PROFILE_ARR <<< "$PROFILE"
for prof in "${PROFILE_ARR[@]}"; do
    prof_upper=$(echo "$prof" | tr '[:lower:]' '[:upper:]')
    CMAKE_ARGS="${CMAKE_ARGS} -DBHARAT_PROFILE_${prof_upper}=1"
done

IFS=',' read -ra PERSONALITY_ARR <<< "$PERSONALITY"
for pers in "${PERSONALITY_ARR[@]}"; do
    pers_upper=$(echo "$pers" | tr '[:lower:]' '[:upper:]')
    CMAKE_ARGS="${CMAKE_ARGS} -DBHARAT_PERSONALITY_${pers_upper}=1"
done

if [ -n "$BOARD" ]; then
    # Map boards to appropriate profiles
    if [[ "$BOARD" == "shakti-c" || "$BOARD" == "shakti-e" || "$BOARD" == "shakti-i" ]]; then
         CMAKE_ARGS="${CMAKE_ARGS} -DBHARAT_BOOT_HW_PROFILE=edge -DBHARAT_ARCH_VARIANT=SHAKTI -DBHARAT_RISCV_SOC_PROFILE=${BOARD}"
    else
         CMAKE_ARGS="${CMAKE_ARGS} -DBHARAT_BOOT_HW_PROFILE=${BOARD}"
    fi
else
    CMAKE_ARGS="${CMAKE_ARGS} -DBHARAT_BOOT_HW_PROFILE=${BOOT_HW}"
fi

if [ -n "$BOOT_HW" ] && [ -z "$BOARD" ]; then
    CMAKE_ARGS="${CMAKE_ARGS} -DBHARAT_BOOT_HW_PROFILE=${BOOT_HW}"
fi

if [ "$PAYLOAD" = true ]; then
    CMAKE_ARGS="${CMAKE_ARGS} -DBHARAT_RISCV_BUILD_PAYLOAD_BIN=ON"
fi

if [ -n "$BOOT_GUI" ]; then
    if [ "$BOOT_GUI" = "true" ] || [ "$BOOT_GUI" = "ON" ] || [ "$BOOT_GUI" = "1" ]; then
        CMAKE_ARGS="${CMAKE_ARGS} -DBHARAT_BOOT_GUI=ON"
    else
        CMAKE_ARGS="${CMAKE_ARGS} -DBHARAT_BOOT_GUI=OFF"
    fi
fi

if [ -n "$TOOLCHAIN_OVERRIDE" ]; then
    CMAKE_ARGS="${CMAKE_ARGS} -DCMAKE_TOOLCHAIN_FILE=${TOOLCHAIN_OVERRIDE}"
else
    if [ "$ARCH" == "riscv64" ]; then
       # Default to standard riscv64-elf unless board implies shakti
       if [[ "$BOARD" == "shakti-c" || "$BOARD" == "shakti-e" || "$BOARD" == "shakti-i" ]]; then
           CMAKE_ARGS="${CMAKE_ARGS} -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/riscv64-shakti-elf.cmake"
       else
           CMAKE_ARGS="${CMAKE_ARGS} -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/riscv64-elf.cmake"
       fi
    else
       CMAKE_ARGS="${CMAKE_ARGS} -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/${ARCH}-elf.cmake"
    fi
fi

echo "Building for Architecture: $ARCH"
echo "Build Directory: $BUILD_DIR"
echo "CMake Arguments: $CMAKE_ARGS"

mkdir -p "${BUILD_DIR}"
cmake -S . -B "${BUILD_DIR}" ${CMAKE_ARGS}
cmake --build "${BUILD_DIR}"

if [ "$RUN" = true ]; then
    echo "Running in QEMU (if configured)..."
    # Basic QEMU run command logic placeholder
    if [ "$ARCH" == "riscv64" ]; then
         qemu-system-riscv64 -M virt -kernel ${BUILD_DIR}/kernel/kernel.elf -nographic
    elif [ "$ARCH" == "x86_64" ]; then
         qemu-system-x86_64 -M q35 -kernel ${BUILD_DIR}/kernel/kernel.elf -nographic
    elif [ "$ARCH" == "arm64" ]; then
         qemu-system-aarch64 -M virt -cpu cortex-a72 -kernel ${BUILD_DIR}/kernel/kernel.elf -nographic
    fi
fi
