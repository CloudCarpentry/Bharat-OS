cmake_minimum_required(VERSION 3.20)
project(BharatOS_Kernel C ASM)

# Add include directories
include_directories(include)

# Core Kernel Subsystems
set(KERNEL_SOURCES
    src/main.c
    src/mm/pmm.c
    src/mm/vmm.c
    src/mm/numa.c
    src/ipc/multikernel.c
    src/ipc/zero_copy_io.c
)

# Architecture selection
set(ARCH "x86_64" CACHE STRING "Target architecture (x86_64, riscv, arm64)")

if(ARCH STREQUAL "x86_64")
    list(APPEND KERNEL_SOURCES src/hal/x86_64/hal_cpu.c)
    # Add target specific flags for x86_64
    set(ARCH_FLAGS -m64 -mno-red-zone -mno-mmx -mno-sse -mno-sse2)
elseif(ARCH STREQUAL "riscv")
    list(APPEND KERNEL_SOURCES src/hal/riscv/hal_cpu.c)
    # Target flags for RISC-V 64-bit (assuming rv64gc / shakti target)
    set(ARCH_FLAGS -target riscv64-unknown-elf -march=rv64gc -mabi=lp64d)
elseif(ARCH STREQUAL "arm64")
    list(APPEND KERNEL_SOURCES src/hal/arm64/hal_cpu.c)
    # Target flags for ARM AArch64
    set(ARCH_FLAGS -target aarch64-unknown-elf -mgeneral-regs-only)
else()
    message(FATAL_ERROR "Unsupported architecture: ${ARCH}")
endif()

# Kernel executable
add_executable(kernel.elf ${KERNEL_SOURCES})

# Target-specific compile options
target_compile_options(kernel.elf PRIVATE -ffreestanding -nostdlib -Wall -Wextra ${ARCH_FLAGS})
target_link_options(kernel.elf PRIVATE -nostdlib ${ARCH_FLAGS})
