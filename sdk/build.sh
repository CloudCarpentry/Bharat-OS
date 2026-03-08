#!/usr/bin/env bash
ARCH=${1:-x86_64}
BUILD_DIR="build/${ARCH}"
cmake -S . -B ${BUILD_DIR} -DCMAKE_TOOLCHAIN_FILE="cmake/toolchains/${ARCH}-elf.cmake"
cmake --build ${BUILD_DIR}
