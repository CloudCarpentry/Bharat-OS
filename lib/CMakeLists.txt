cmake_minimum_required(VERSION 3.20)
project(BharatOS_Lib C ASM)

# Add include directories for libc/posix
include_directories(posix/include)

# The actual C library implementation (e.g., musl) would be integrated here.
# For now, we create a dummy target to satisfy the build system.
add_library(c_posix_stub STATIC posix/stub.c)
target_compile_options(c_posix_stub PRIVATE -ffreestanding -nostdlib -Wall)

# URPC Library
add_library(urpc STATIC urpc/urpc.c)
target_include_directories(urpc PUBLIC urpc)
target_compile_options(urpc PRIVATE -ffreestanding -nostdlib -Wall)
add_subdirectory(elf)
add_subdirectory(syscall)

# Phase 1: Foundational Native Libraries
add_subdirectory(cap)
add_subdirectory(ipc)
add_subdirectory(runtime)
if(BHARAT_BUILD_NETWORK_STUBS)
    add_subdirectory(net)
    add_subdirectory(packet)
endif()
