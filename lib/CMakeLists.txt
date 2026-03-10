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
