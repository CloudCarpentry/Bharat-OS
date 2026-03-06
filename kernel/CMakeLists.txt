cmake_minimum_required(VERSION 3.20)
project(BharatOS_Kernel C ASM)

# ── Toolchain file provides all compiler/linker settings.
# Pass -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchains/<arch>-elf.cmake at configure time.
# See BUILD.md for exact commands for each platform.

include_directories(include)

# ── Architecture selection ──────────────────────────────────────────────────
# Inferred from the toolchain file's CMAKE_SYSTEM_PROCESSOR, or set explicitly.
if(CMAKE_SYSTEM_PROCESSOR MATCHES "riscv")
    set(ARCH "riscv")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64")
    set(ARCH "arm64")
else()
    set(ARCH "x86_64")
endif()

# ── Boot and HAL sources (arch-specific) ────────────────────────────────────
if(ARCH STREQUAL "x86_64")
    set(BOOT_SRC src/boot/x86_64/boot.S)
    set(ARCH_HAL src/hal/x86_64/hal_cpu.c)
elseif(ARCH STREQUAL "riscv")
    set(BOOT_SRC "")  # RISC-V SBI boot stub — future milestone
    set(ARCH_HAL src/hal/riscv/hal_cpu.c)
elseif(ARCH STREQUAL "arm64")
    set(BOOT_SRC "")  # ARM64 EFI boot stub — future milestone
    set(ARCH_HAL src/hal/arm64/hal_cpu.c)
endif()

# ── Core kernel sources ──────────────────────────────────────────────────────
set(KERNEL_SRCS
    ${BOOT_SRC}
    src/main.c
    ${ARCH_HAL}
    src/mm/pmm.c
    src/mm/vmm.c
    src/mm/numa.c
    src/ipc/multikernel.c
    src/lib/string.c
)

# ── Kernel ELF ──────────────────────────────────────────────────────────────
add_executable(kernel.elf ${KERNEL_SRCS})

set_target_properties(kernel.elf PROPERTIES
    OUTPUT_NAME "kernel.elf"
    SUFFIX ""
)

# Extra warnings for C sources only
target_compile_options(kernel.elf PRIVATE
    $<$<COMPILE_LANGUAGE:C>:-Wall>
    $<$<COMPILE_LANGUAGE:C>:-Wextra>
    $<$<COMPILE_LANGUAGE:C>:-Werror=implicit-function-declaration>
)

# Linker script
# Linker script + options
target_link_options(kernel.elf PRIVATE
    "-T" "${CMAKE_CURRENT_SOURCE_DIR}/linker.ld"
    "--build-id=none"
    "--no-dynamic-linker"
)
