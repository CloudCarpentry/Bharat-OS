#!/usr/bin/env bash

# Strict error handling
set -euo pipefail

# Print help message
usage() {
    echo "Bharat-OS SDK Build Script"
    echo "Usage: $0 [OPTIONS]"
    echo ""
    echo "Options:"
    echo "  --arch <arch>     Target architecture (default: x86_64, options: x86_64, arm64, riscv64)"
    echo "  --clean           Clean the build directory before building"
    echo "  --rebuild         Alias for --clean followed by a build"
    echo "  --help, -h        Show this help message"
    echo ""
    echo "Example:"
    echo "  $0 --arch arm64 --clean"
    exit 1
}

ARCH="x86_64"
CLEAN=false

# Parse arguments
while [[ $# -gt 0 ]]; do
    case "$1" in
        --arch)
            if [ -z "${2:-}" ]; then
                echo "Error: --arch requires an argument."
                usage
            fi
            ARCH="$2"
            shift 2
            ;;
        --clean|--rebuild)
            CLEAN=true
            shift
            ;;
        --help|-h)
            usage
            ;;
        *)
            echo "Unknown option: $1"
            usage
            ;;
    esac
done

# Environment validation
if ! command -v cmake &> /dev/null; then
    echo "Error: 'cmake' is not installed or not in PATH."
    exit 1
fi

if ! command -v ninja &> /dev/null && ! command -v make &> /dev/null; then
    echo "Error: Neither 'ninja' nor 'make' is installed. Please install a build tool."
    exit 1
fi

BUILD_DIR="build/${ARCH}"
TOOLCHAIN_FILE="../../cmake/toolchains/${ARCH}-elf.cmake"

if [ ! -f "${TOOLCHAIN_FILE}" ]; then
    echo "Error: Toolchain file not found at ${TOOLCHAIN_FILE}"
    echo "Ensure you are running this from the user/sdk/ directory and the toolchain exists."
    exit 1
fi

if [ "$CLEAN" = true ]; then
    echo "Cleaning build directory: ${BUILD_DIR}"
    rm -rf "${BUILD_DIR}"
fi

echo "========================================"
echo " Building Bharat-OS SDK                 "
echo " Architecture: ${ARCH}"
echo " Build Dir:    ${BUILD_DIR}"
echo "========================================"

# Configure CMake
echo "Configuring CMake..."
if ! cmake -S . -B "${BUILD_DIR}" -DCMAKE_TOOLCHAIN_FILE="${TOOLCHAIN_FILE}"; then
    echo "Error: CMake configuration failed."
    echo "Troubleshooting:"
    echo " - Check if your compiler (clang/gcc) supports ${ARCH}."
    echo " - Verify the toolchain file at ${TOOLCHAIN_FILE}."
    exit 1
fi

# Build
echo "Building..."
if ! cmake --build "${BUILD_DIR}"; then
    echo "Error: Build failed."
    exit 1
fi

echo "========================================"
echo " Build successful!                      "
echo " SDK and sample app are in ${BUILD_DIR} "
echo "========================================"
