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
DUAL_SERIAL=false

while [[ $# -gt 0 ]]; do
    case "$1" in
        -Arch|--arch)
            ARCH="$2"
            ARCH_SET=true
            shift 2
            ;;
        --arch=*)
            ARCH="${1#*=}"
            ARCH_SET=true
            shift
            ;;
        -Board|--board)
            BOARD="$2"
            shift 2
            ;;
        --board=*)
            BOARD="${1#*=}"
            shift
            ;;
        -Toolchain|--toolchain)
            TOOLCHAIN_OVERRIDE="$2"
            shift 2
            ;;
        --toolchain=*)
            TOOLCHAIN_OVERRIDE="${1#*=}"
            shift
            ;;
        -Clean|--clean)
            CLEAN=true
            shift
            ;;
        -Run|--run)
            RUN=true
            shift
            ;;
        -Debug|--debug)
            DEBUG=true
            shift
            ;;
        -Payload|--payload)
            PAYLOAD=true
            shift
            ;;
        -Flash|--flash)
            FLASH=true
            shift
            ;;
        -BootGui|--boot-gui)
            BOOT_GUI="$2"
            shift 2
            ;;
        --boot-gui=*)
            BOOT_GUI="${1#*=}"
            shift
            ;;
        -Hw|-HardwareProfile|--hw)
            BOOT_HW="$2"
            shift 2
            ;;
        --hw=*)
            BOOT_HW="${1#*=}"
            shift
            ;;
        -Machine|--machine)
            MACHINE="$2"
            shift 2
            ;;
        --machine=*)
            MACHINE="${1#*=}"
            shift
            ;;
        -Tier|--tier)
            BOOT_TIER="$2"
            shift 2
            ;;
        --tier=*)
            BOOT_TIER="${1#*=}"
            shift
            ;;
        -Profile|--profile)
            PROFILE="$2"
            shift 2
            ;;
        --profile=*)
            PROFILE="${1#*=}"
            shift
            ;;
        -Personality|--personality)
            PERSONALITY="$2"
            shift 2
            ;;
        --personality=*)
            PERSONALITY="${1#*=}"
            shift
            ;;
        -DualSerial|--dual-serial)
            DUAL_SERIAL=true
            shift
            ;;
        *)
            if [[ ! "$1" == -* ]] && [[ -z "$ARCH_SET" ]]; then
                ARCH="$1"
                ARCH_SET=true
                shift
            else
                echo "Unknown option: $1"
                shift
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
if [ "${#PROFILE_ARR[@]}" -gt 1 ]; then
    echo "Warning: multiple --profile values are not supported; using first value: ${PROFILE_ARR[0]}"
fi
PROFILE_SELECTED="${PROFILE_ARR[0]}"
PROFILE_UPPER=$(echo "$PROFILE_SELECTED" | tr '[:lower:]' '[:upper:]')
CMAKE_ARGS="${CMAKE_ARGS} -DBHARAT_DEVICE_PROFILE=${PROFILE_UPPER}"

IFS=',' read -ra PERSONALITY_ARR <<< "$PERSONALITY"
if [ "${#PERSONALITY_ARR[@]}" -gt 1 ]; then
    echo "Warning: multiple --personality values are not supported; using first value: ${PERSONALITY_ARR[0]}"
fi
PERSONALITY_SELECTED="${PERSONALITY_ARR[0]}"
PERSONALITY_UPPER=$(echo "$PERSONALITY_SELECTED" | tr '[:lower:]' '[:upper:]')
CMAKE_ARGS="${CMAKE_ARGS} -DBHARAT_PERSONALITY_PROFILE=${PERSONALITY_UPPER}"

if [ -n "$BOARD" ]; then
    CMAKE_ARGS="${CMAKE_ARGS} -DBHARAT_TARGET_BOARD=${BOARD}"
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
elif [ "$ARCH" == "riscv64" ]; then
    CMAKE_ARGS="${CMAKE_ARGS} -DBHARAT_RISCV_BUILD_PAYLOAD_BIN=OFF"
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
    echo "Running in QEMU..."

    KERNEL_BIN="${BUILD_DIR}/kernel/kernel.elf"

    # x86_64: convert to 32-bit ELF required by Multiboot
    if [ "$ARCH" == "x86_64" ]; then
        KERNEL_BIN="${BUILD_DIR}/kernel/kernel32.elf"
        llvm-objcopy -I elf64-x86-64 -O elf32-i386 \
            "${BUILD_DIR}/kernel/kernel.elf" "$KERNEL_BIN"
    elif [ "$ARCH" == "arm64" ]; then
        KERNEL_BIN="${BUILD_DIR}/kernel/Image"
        llvm-objcopy -O binary \
            "${BUILD_DIR}/kernel/kernel.elf" "$KERNEL_BIN"
    fi

    if [ "$ARCH" == "x86_64" ]; then
        GUI_ARGS=""
        SERIAL_ARGS="-serial stdio"
        if [ "$BOOT_GUI" = "ON" ] || [ "$BOOT_GUI" = "true" ] || [ "$BOOT_GUI" = "1" ]; then
            GUI_ARGS="-vga std"
            if [ "$DUAL_SERIAL" = true ]; then
                SERIAL_ARGS="-serial stdio -serial vc"
            else
                SERIAL_ARGS="-serial vc"
            fi
        else
            GUI_ARGS="-nographic"
        fi
        qemu-system-x86_64 \
            -kernel "$KERNEL_BIN" \
            -m 256M \
            $SERIAL_ARGS \
            -no-reboot \
            $GUI_ARGS

    elif [ "$ARCH" == "riscv64" ]; then
        GUI_ARGS=""
        SERIAL_ARGS="-serial stdio"
        if [ "$BOOT_GUI" = "ON" ] || [ "$BOOT_GUI" = "true" ] || [ "$BOOT_GUI" = "1" ]; then
            # Route serial output to the virtual console in the QEMU graphical window.
            # Without a firmware or GPU, we drop virtio-gpu and force QEMU to display the 'vc' directly on the main window tab.
            GUI_ARGS="-display gtk"
            if [ "$DUAL_SERIAL" = true ]; then
                SERIAL_ARGS="-serial stdio -serial vc"
            else
                SERIAL_ARGS="-serial vc"
            fi
        else
            GUI_ARGS="-nographic"
        fi
        qemu-system-riscv64 \
            -machine virt \
            -kernel "$KERNEL_BIN" \
            -m 256M \
            $SERIAL_ARGS \
            -no-reboot \
            $GUI_ARGS

    elif [ "$ARCH" == "arm64" ]; then
        GUI_ARGS=""
        SERIAL_ARGS="-serial stdio"
        if [ "$BOOT_GUI" = "ON" ] || [ "$BOOT_GUI" = "true" ] || [ "$BOOT_GUI" = "1" ]; then
            # Route serial output to the virtual console in the QEMU graphical window.
            # Without a firmware or GPU, we drop virtio-gpu and force QEMU to display the 'vc' directly on the main window tab.
            GUI_ARGS="-vga none -display gtk"
            if [ "$DUAL_SERIAL" = true ]; then
                SERIAL_ARGS="-serial stdio -serial vc"
            else
                SERIAL_ARGS="-serial vc"
            fi
        else
            GUI_ARGS="-nographic"
        fi
        qemu-system-aarch64 \
            -machine virt \
            -cpu cortex-a72 \
            -kernel "$KERNEL_BIN" \
            -m 256M \
            $SERIAL_ARGS \
            -no-reboot \
            $GUI_ARGS
    fi
fi
