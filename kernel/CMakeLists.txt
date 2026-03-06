cmake_minimum_required(VERSION 3.20)
project(BharatOS_Kernel C ASM)

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# ── Toolchain file provides all compiler/linker settings.
# Pass -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchains/<arch>-elf.cmake at configure time.
# See BUILD.md for exact commands for each platform.

include_directories(include)

option(BHARAT_BOOT_GUI "Enable boot-time GUI handoff metadata" ON)
set(BHARAT_BOOT_HW_PROFILE "generic" CACHE STRING "Boot hardware profile (generic, desktop, server, vm, laptop)")
set_property(CACHE BHARAT_BOOT_HW_PROFILE PROPERTY STRINGS generic desktop server vm laptop)

if(BHARAT_BOOT_GUI)
    add_compile_definitions(BHARAT_BOOT_GUI=1)
else()
    add_compile_definitions(BHARAT_BOOT_GUI=0)
endif()

if(NOT BHARAT_BOOT_HW_PROFILE MATCHES "^(generic|desktop|server|vm|laptop)$")
    message(FATAL_ERROR "Invalid BHARAT_BOOT_HW_PROFILE='${BHARAT_BOOT_HW_PROFILE}'. Allowed: generic, desktop, server, vm, laptop")
endif()

add_compile_definitions(BHARAT_BOOT_HW_PROFILE_${BHARAT_BOOT_HW_PROFILE}=1)

# ── Architecture selection ──────────────────────────────────────────────────
# Inferred from the toolchain file's CMAKE_SYSTEM_PROCESSOR, or set explicitly.
if(CMAKE_SYSTEM_PROCESSOR MATCHES "riscv")
    set(ARCH "riscv64")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64")
    set(ARCH "arm64")
else()
    set(ARCH "x86_64")
endif()

# ── Boot and HAL sources (arch-specific) ────────────────────────────────────
if(ARCH STREQUAL "x86_64")
    set(BOOT_SRC src/boot/x86_64/boot.S)
    set(ARCH_HAL src/hal/x86_64/hal_cpu.c)
elseif(ARCH STREQUAL "riscv64")
    set(BOOT_SRC "")  # RISC-V SBI boot stub — future milestone
    set(ARCH_HAL src/hal/riscv/hal_cpu.c)
elseif(ARCH STREQUAL "arm64")
    set(BOOT_SRC "")  # ARM64 EFI boot stub — future milestone
    set(ARCH_HAL src/hal/arm64/hal_cpu.c)
endif()

if(ARCH STREQUAL "x86_64")
    set(LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/linker.ld")
elseif(ARCH STREQUAL "riscv64")
    set(LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/linker_riscv64.ld")
elseif(ARCH STREQUAL "arm64")
    set(LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/linker_arm64.ld")
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
    $<$<COMPILE_LANGUAGE:C>:-ffreestanding>
    $<$<COMPILE_LANGUAGE:C>:-fno-builtin>
    $<$<COMPILE_LANGUAGE:C>:-fno-stack-protector>
    $<$<COMPILE_LANGUAGE:C>:-fno-pic>
    $<$<COMPILE_LANGUAGE:C>:-Wall>
    $<$<COMPILE_LANGUAGE:C>:-Wextra>
    $<$<COMPILE_LANGUAGE:C>:-Werror=implicit-function-declaration>
)

target_link_options(kernel.elf PRIVATE
    "-nostdlib"
    "-static"
    "LINKER:-no-pie"
    "LINKER:--fatal-warnings"
)

# Linker script
# Linker script + options
target_link_options(kernel.elf PRIVATE
    "LINKER:-T,${LINKER_SCRIPT}"
    "LINKER:--build-id=none"
    "LINKER:--no-dynamic-linker"
)
