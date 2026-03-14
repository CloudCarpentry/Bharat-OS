cmake_minimum_required(VERSION 3.20)
project(BharatOS_Kernel C ASM)

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# ── Toolchain file provides all compiler/linker settings.
# Pass -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchains/<arch>-elf.cmake at configure time.
# See BUILD.md for exact commands for each platform.

include_directories(include)
include_directories(include/generated)

# ── Configuration Header ────────────────────────────────────────────────────
# These variables are normally set in the root CMakeLists.txt. 
# For kernel-only builds, we provide defaults if not set.
if(NOT DEFINED BHARAT_ARCH_X86)
    set(BHARAT_ARCH_X86 1)
endif()
if(NOT DEFINED BHARAT_ARCH_64BIT)
    set(BHARAT_ARCH_64BIT 1)
endif()
if(NOT DEFINED BHARAT_ARCH_VARIANT_GENERIC)
    set(BHARAT_ARCH_VARIANT_GENERIC 1)
endif()
if(NOT DEFINED BHARAT_PROFILE_DESKTOP)
    set(BHARAT_PROFILE_DESKTOP 1)
endif()
if(NOT DEFINED BHARAT_PERSONALITY_NONE)
    set(BHARAT_PERSONALITY_NONE 1)
endif()
if(NOT BHARAT_MAX_CORES)
    set(BHARAT_MAX_CORES 32)
endif()

# Multikernel Configuration Defaults
if(NOT DEFINED CONFIG_MULTIKERNEL)
    set(CONFIG_MULTIKERNEL 1)
endif()
if(NOT DEFINED CONFIG_HETEROGENEOUS_CPU)
    set(CONFIG_HETEROGENEOUS_CPU 1)
endif()
if(NOT DEFINED CONFIG_URPC)
    set(CONFIG_URPC 1)
endif()

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/include/bharat_config.h.in
    ${CMAKE_CURRENT_SOURCE_DIR}/include/generated/bharat_config.h
)

option(BHARAT_BOOT_GUI "Enable boot-time GUI handoff metadata" ON)

if(NOT DEFINED BHARAT_BOOT_HW_PROFILE OR "${BHARAT_BOOT_HW_PROFILE}" STREQUAL "")
    message(FATAL_ERROR "BHARAT_BOOT_HW_PROFILE must not be empty")
endif()

if(BHARAT_BOOT_GUI)
    add_compile_definitions(BHARAT_BOOT_GUI=1)
else()
    add_compile_definitions(BHARAT_BOOT_GUI=0)
endif()

string(TOLOWER "${BHARAT_BOOT_HW_PROFILE}" KERNEL_PROFILE_LOWER)

if(NOT KERNEL_PROFILE_LOWER MATCHES "^(generic|desktop|server|vm|laptop|edge|rtos|mobile|datacenter|network_appliance)$")
    message(FATAL_ERROR "Invalid BHARAT_BOOT_HW_PROFILE='${KERNEL_PROFILE_LOWER}'. Allowed: generic, desktop, server, vm, laptop, edge, rtos, mobile, datacenter, network_appliance")
endif()

add_compile_definitions(BHARAT_BOOT_HW_PROFILE_${KERNEL_PROFILE_LOWER}=1)

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
    set(ARCH_HAL src/hal/x86_64/hal_cpu.c src/hal/x86_64/trap_entry.S src/hal/x86_64/vmm.c src/hal/x86_64/vtd_iommu.c src/hal/x86_64/apic.c src/hal/x86_64/timer.c)
    set(ARCH_CONTEXT_SWITCH src/arch/x86_64/context_switch.c src/arch/x86_64/context_switch.S)
elseif(ARCH STREQUAL "riscv64")
    set(BOOT_SRC src/boot/riscv64/boot.S)
    set(ARCH_HAL src/hal/riscv/hal_cpu.c src/hal/riscv/trap_entry.S src/hal/riscv/vmm.c src/hal/riscv/iopmp.c src/hal/riscv/plic.c src/hal/riscv/sbi_timer_ipi.c)
    set(ARCH_HAL_EXTRA src/hal/riscv/shakti_bsp.c)
    set(ARCH_CONTEXT_SWITCH src/arch/riscv64/context_switch.c src/arch/riscv64/context_switch.S)
elseif(ARCH STREQUAL "arm64")
    set(BOOT_SRC src/boot/arm64/boot.S)
    set(ARCH_HAL src/hal/arm64/hal_cpu.c src/hal/arm64/trap_entry.S src/hal/arm64/vmm.c src/hal/arm64/smmu.c src/hal/arm64/gicv3.c src/hal/arm64/timer_generic.c)
    set(ARCH_CONTEXT_SWITCH src/arch/arm64/context_switch.c src/arch/arm64/context_switch.S)
    set(ARCH_HAL_EXTRA src/hal/arm64/topology_fdt.c)
endif()

if(ARCH STREQUAL "x86_64")
    set(LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/linker.ld")
elseif(ARCH STREQUAL "riscv64")
    set(LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/linker_riscv64.ld")
elseif(ARCH STREQUAL "arm64")
    set(LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/linker_arm64.ld")
endif()

if(ARCH STREQUAL "riscv64")
    option(BHARAT_RISCV_BUILD_PAYLOAD_BIN "Emit payload.bin from kernel.elf for OpenSBI packaging" ON)
    set(BHARAT_RISCV_SOC_PROFILE "qemu-virt" CACHE STRING "RISC-V SoC profile (qemu-virt, shakti-e, shakti-c, shakti-i)")
    set_property(CACHE BHARAT_RISCV_SOC_PROFILE PROPERTY STRINGS qemu-virt shakti-e shakti-c shakti-i)

    if(NOT BHARAT_RISCV_SOC_PROFILE MATCHES "^(qemu-virt|shakti-e|shakti-c|shakti-i)$")
        message(FATAL_ERROR "Invalid BHARAT_RISCV_SOC_PROFILE='${BHARAT_RISCV_SOC_PROFILE}'. Allowed: qemu-virt, shakti-e, shakti-c, shakti-i")
    endif()

    add_compile_definitions(BHARAT_RISCV_SOC_PROFILE_STR="${BHARAT_RISCV_SOC_PROFILE}")
endif()

# ── Core kernel sources ──────────────────────────────────────────────────────
set(KERNEL_SRCS
    ${BOOT_SRC}
    src/boot/boot_args.c
    src/hal/arm32_mpu_vmm_mock.c
    src/main.c
    src/trace/trace.c
    src/subsystem/profile.c
    src/arch/capabilities.c
    src/profile/${KERNEL_PROFILE_LOWER}/profile.c
    $<$<STREQUAL:${ARCH},x86_64>:src/board/qemu-virt-x86_64/board.c>
    $<$<STREQUAL:${ARCH},arm64>:src/board/bcm2711-rpi4/board.c>
    $<$<AND:$<STREQUAL:${ARCH},riscv64>,$<STREQUAL:${BHARAT_RISCV_SOC_PROFILE},qemu-virt>>:src/board/qemu-virt-riscv64/board.c>
    $<$<AND:$<STREQUAL:${ARCH},riscv64>,$<NOT:$<STREQUAL:${BHARAT_RISCV_SOC_PROFILE},qemu-virt>>>:src/board/shakti-ref/board.c>
    src/ai_kernel_bridge.c
    src/ai_sched.c
    src/panic.c
    src/sched.c
    src/sched_deg.c
    src/algo_matrix.c
    ${ARCH_HAL}
    src/hal/common/fdt_parser.c
    ${ARCH_HAL_EXTRA}
    ${ARCH_CONTEXT_SWITCH}
    src/arch/${ARCH}/cpu_caps.c
    src/arch/${ARCH}/ext_state.c
    src/hal/mmu_init.c
    src/hal/iommu_stub.c
    src/mm/pmm.c
    src/mm/early_alloc.c
    src/mm/slab.c
    src/mm/kmem_aligned.c
    src/mm/vmm.c
    src/mm/numa.c
    src/mm/zswap.c
    src/security/isolation.c
    src/security/credentials.c
    src/security/policy.c
    src/security/audit.c
    src/security/secure_boot.c
    src/security/vfio.c
    src/ipc/endpoint_ipc.c
    src/ipc/async_ipc.c
    src/ipc/ipc_timeout.c
    src/capability.c
    src/ipc/multikernel.c
    src/ipc/mk_dispatch.c
    src/ipc/mk_proto.c
    src/lib/string.c
    src/stack_protector.c
    src/device/builtin_drivers.c
    src/device/device_manager.c
    src/device/pci.c

    src/hal/interrupt_common.c
    src/hal/timer_common.c

    src/power_thermal_perf.c
    src/trap/trap.c
    src/multicore.c
    src/fs/vfs.c
    src/fs/mount.c
    src/fs/file.c
    src/fs/namespace.c
    src/fs/path.c
    src/fs/block.c
    src/fs/object_store.c
    src/fs/blob_backend.c
    src/apps/hello_world.c
    src/apps/kernel_tester.c
    src/tests/ktest_pmm.c
    src/tests/ktest_slab.c
    src/tests/ktest_runner.c
)

# ── Kernel ELF ──────────────────────────────────────────────────────────────
add_executable(kernel.elf ${KERNEL_SRCS})

target_link_libraries(kernel.elf subsys_manager)

set_target_properties(kernel.elf PROPERTIES
    OUTPUT_NAME "kernel.elf"
    SUFFIX ""
)

# Extra warnings for C sources only
target_compile_options(kernel.elf PRIVATE
    $<$<COMPILE_LANGUAGE:C>:-ffreestanding>
    $<$<COMPILE_LANGUAGE:C>:-fno-builtin>
    $<$<COMPILE_LANGUAGE:C>:-fno-pic>
    $<$<COMPILE_LANGUAGE:C>:-Wall>
    $<$<COMPILE_LANGUAGE:C>:-Wextra>
    $<$<COMPILE_LANGUAGE:C>:-Werror=implicit-function-declaration>
)

option(BHARAT_KERNEL_HARDENING "Enable kernel hardening compile/link options" ON)
option(BHARAT_KERNEL_ASLR "Build kernel as PIE for KASLR-friendly relocation experiments" OFF)

if(BHARAT_KERNEL_HARDENING)
    target_compile_options(kernel.elf PRIVATE
        $<$<COMPILE_LANGUAGE:C>:-fstack-protector-strong>
        $<$<COMPILE_LANGUAGE:C>:-fno-strict-aliasing>
        $<$<COMPILE_LANGUAGE:C>:-fno-delete-null-pointer-checks>
    )
    target_link_options(kernel.elf PRIVATE
        "-z"
        "noexecstack"
    )
else()
    target_compile_options(kernel.elf PRIVATE
        $<$<COMPILE_LANGUAGE:C>:-fno-stack-protector>
    )
endif()

if(BHARAT_KERNEL_ASLR)
    target_compile_options(kernel.elf PRIVATE
        $<$<COMPILE_LANGUAGE:C>:-fpie>
    )
    target_link_options(kernel.elf PRIVATE
        "LINKER:--pie"
    )
else()
    target_compile_options(kernel.elf PRIVATE
        $<$<COMPILE_LANGUAGE:C>:-fno-pie>
    )
    target_link_options(kernel.elf PRIVATE
        "LINKER:--no-pie"
    )
endif()

if(ARCH STREQUAL "x86_64")
    target_compile_options(kernel.elf PRIVATE
        $<$<COMPILE_LANGUAGE:C>:-m64>
        $<$<COMPILE_LANGUAGE:C>:-mno-red-zone>
    )
    target_link_options(kernel.elf PRIVATE
        "LINKER:-m,elf_x86_64"
    )
elseif(ARCH STREQUAL "riscv64")
    target_compile_options(kernel.elf PRIVATE
        $<$<COMPILE_LANGUAGE:C>:-mcmodel=medany>
    )
endif()

# ── Linker options
# Use LINKER: so CMake can translate options correctly when linking via the
# compiler driver across clang/gcc and host/cross presets.
target_link_options(kernel.elf PRIVATE
    "-nostdlib"
    "LINKER:--fatal-warnings"
    "LINKER:-T,${LINKER_SCRIPT}"
    "LINKER:--build-id=none"
    "LINKER:--no-dynamic-linker"
)


if(ARCH STREQUAL "riscv64" AND BHARAT_RISCV_BUILD_PAYLOAD_BIN)
    if(NOT CMAKE_OBJCOPY)
        find_program(CMAKE_OBJCOPY NAMES llvm-objcopy riscv64-unknown-elf-objcopy)
    endif()

    if(NOT CMAKE_OBJCOPY)
        message(WARNING "objcopy not found; payload.bin target disabled")
    else()
        add_custom_command(
            OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/payload.bin"
            COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:kernel.elf> "${CMAKE_CURRENT_BINARY_DIR}/payload.bin"
            DEPENDS kernel.elf
            COMMENT "Generating OpenSBI payload.bin from kernel.elf"
            VERBATIM
        )
        add_custom_target(kernel.payload.bin ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/payload.bin")
    endif()
endif()
