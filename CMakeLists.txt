cmake_minimum_required(VERSION 3.20)
project(BharatOS VERSION 0.8.0 LANGUAGES C ASM)

enable_testing()

set(BHARAT_CMAKE_ROOT "${CMAKE_SOURCE_DIR}/delivery/cmake")
if(NOT EXISTS "${BHARAT_CMAKE_ROOT}")
    message(FATAL_ERROR "Missing canonical CMake root: ${BHARAT_CMAKE_ROOT}")
endif()

# Make the modules directory visible to CMake
list(APPEND CMAKE_MODULE_PATH "${BHARAT_CMAKE_ROOT}/modules")

# Load Common Build Policies
include(BharatCommon)
include(BharatProfiles)
include(BharatPersonality)
include(BharatComponentPolicy)
include(${BHARAT_CMAKE_ROOT}/BharatConstraints.cmake)

# Versioning Identity System: Generate and load from manifest
set(BHARAT_MANIFEST_CANDIDATES
    "${CMAKE_SOURCE_DIR}/delivery/release/manifest/os-release.json"
    "${CMAKE_SOURCE_DIR}/release/manifest/os-release.json"
)
set(BHARAT_MANIFEST_JSON "")
foreach(_manifest_path IN LISTS BHARAT_MANIFEST_CANDIDATES)
    if(EXISTS "${_manifest_path}")
        set(BHARAT_MANIFEST_JSON "${_manifest_path}")
        if(_manifest_path MATCHES "/release/manifest/os-release.json$"
           AND NOT _manifest_path MATCHES "/delivery/release/manifest/os-release.json$")
            message(WARNING "[migration-warning] Using legacy manifest path: ${_manifest_path}")
        endif()
        break()
    endif()
endforeach()
if(NOT BHARAT_MANIFEST_JSON)
    message(FATAL_ERROR "No os-release manifest found in delivery/release/manifest or release/manifest")
endif()
set(BHARAT_GEN_SCRIPT "${CMAKE_SOURCE_DIR}/tools/gen_manifest.py")
set(BHARAT_GEN_DIR "${CMAKE_BINARY_DIR}/generated")
set(BHARAT_GEN_INCLUDE_DIR "${BHARAT_GEN_DIR}/include")

file(MAKE_DIRECTORY "${BHARAT_GEN_DIR}")
file(MAKE_DIRECTORY "${BHARAT_GEN_INCLUDE_DIR}/bharat")

# Process static IDL generation explicitly early to guarantee header availability
set(BHARAT_IDL_GEN_SCRIPT "${CMAKE_SOURCE_DIR}/tools/bidl/bidlc.py")
set(BHARAT_IDL_ROOT_CANDIDATES
    "${CMAKE_SOURCE_DIR}/interface/idl"
    "${CMAKE_SOURCE_DIR}/idl"
)
set(BHARAT_IDL_ROOT "")
foreach(_idl_root IN LISTS BHARAT_IDL_ROOT_CANDIDATES)
    if(EXISTS "${_idl_root}")
        set(BHARAT_IDL_ROOT "${_idl_root}")
        if(_idl_root MATCHES "/idl$" AND NOT _idl_root MATCHES "/interface/idl$")
            message(WARNING "[migration-warning] Using legacy IDL root: ${_idl_root}")
        endif()
        break()
    endif()
endforeach()
if(NOT BHARAT_IDL_ROOT)
    message(FATAL_ERROR "No IDL root found in interface/idl or idl")
endif()
find_package(Python3 REQUIRED COMPONENTS Interpreter)

# Generate monitor service IDL
execute_process(
    COMMAND "${Python3_EXECUTABLE}" "${BHARAT_IDL_GEN_SCRIPT}"
            "${BHARAT_IDL_ROOT}/monitor_v1.bidl"
            "${BHARAT_GEN_INCLUDE_DIR}"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    RESULT_VARIABLE IDL_MONITOR_RESULT
)
if(NOT IDL_MONITOR_RESULT EQUAL 0)
    message(WARNING "Failed to generate monitor IDL. Build may fail if headers are missing.")
endif()

# Generate cap service IDL
execute_process(
    COMMAND "${Python3_EXECUTABLE}" "${BHARAT_IDL_GEN_SCRIPT}"
            "${BHARAT_IDL_ROOT}/cap_v1.bidl"
            "${BHARAT_GEN_INCLUDE_DIR}"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    RESULT_VARIABLE IDL_CAP_RESULT
)
if(NOT IDL_CAP_RESULT EQUAL 0)
    message(WARNING "Failed to generate cap IDL. Build may fail if headers are missing.")
endif()

find_package(Python3 REQUIRED COMPONENTS Interpreter)

execute_process(
    COMMAND "${Python3_EXECUTABLE}" "${BHARAT_GEN_SCRIPT}"
            "${BHARAT_MANIFEST_JSON}"
            "${BHARAT_GEN_DIR}"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    RESULT_VARIABLE BHARAT_GEN_RESULT
)

if(NOT BHARAT_GEN_RESULT EQUAL 0)
    message(FATAL_ERROR "gen_manifest.py failed with code ${BHARAT_GEN_RESULT}")
endif()

include("${BHARAT_GEN_DIR}/BharatVersion.cmake")

# Provide an interface target so components can inherit generated includes safely
add_library(bharat_generated_includes INTERFACE)
target_include_directories(bharat_generated_includes INTERFACE "${BHARAT_GEN_INCLUDE_DIR}")

# Architecture selection
bharat_set_arch_defaults()
bharat_apply_arch_flags()

# Device Profile
set(BHARAT_DEVICE_PROFILE "DESKTOP" CACHE STRING "Target device profile (DESKTOP, MOBILE, EDGE, DATACENTER, NETWORK_APPLIANCE, DRONE, ROBOT, RTOS, AUTOMOTIVE_ECU, AUTOMOTIVE_INFOTAINMENT)")
set_property(CACHE BHARAT_DEVICE_PROFILE PROPERTY STRINGS DESKTOP MOBILE EDGE DATACENTER NETWORK_APPLIANCE DRONE ROBOT RTOS AUTOMOTIVE_ECU AUTOMOTIVE_INFOTAINMENT)

# Personality Profile
set(BHARAT_PERSONALITY_PROFILE "NATIVE" CACHE STRING "Target personality profile (NATIVE, LINUX, ANDROID, WINDOWS, MAC)")
set_property(CACHE BHARAT_PERSONALITY_PROFILE PROPERTY STRINGS NATIVE LINUX ANDROID WINDOWS MAC)

# Default Init Profile
set(BHARAT_DEFAULT_INIT_PROFILE "DESKTOP" CACHE STRING "Target default init profile (TINY, SMALL, EMBEDDED_RICH, MOBILE, DESKTOP, DRONE)")
set_property(CACHE BHARAT_DEFAULT_INIT_PROFILE PROPERTY STRINGS TINY SMALL EMBEDDED_RICH MOBILE DESKTOP DRONE)

# Board selection metadata (provided by build scripts or manual configure)
set(BHARAT_TARGET_BOARD "" CACHE STRING "Target board identifier (e.g., qemu-virt-riscv64, shakti-c)")

# ISA Extensions
set(BHARAT_ISA_BASELINE "" CACHE STRING "Target ISA baseline (e.g., x86_64_v2, rv64gc)")
set(BHARAT_ISA_FEATURES "" CACHE STRING "Target ISA features as a semicolon-separated list (e.g., avx2;fma;aes or v;zba;zbb)")

# Hardware Accelerators
set(BHARAT_ACCELERATOR_CLASSES "" CACHE STRING "Target accelerator classes as a semicolon-separated list (e.g., gpu;npu;dsp;crypto;vpu)")

# Execution Mode Framework Integration
set(BHARAT_EXECUTION_MODE_MODULE "${BHARAT_CMAKE_ROOT}/BharatExecutionMode.cmake")
if(EXISTS "${BHARAT_EXECUTION_MODE_MODULE}")
    include("${BHARAT_EXECUTION_MODE_MODULE}")
else()
    message(WARNING "Optional execution mode module not found: ${BHARAT_EXECUTION_MODE_MODULE}. Continuing with in-tree defaults.")
endif()

# Memory Capability Options
option(BHARAT_ENABLE_MMU "Enable MMU support" ON)
option(BHARAT_ENABLE_MPU "Enable MPU support" OFF)
option(BHARAT_ENABLE_ADVANCED_VM "Enable advanced virtual memory" ON)
option(BHARAT_ENABLE_IOMMU "Enable IOMMU support" OFF)
option(BHARAT_ENABLE_DMA_MAP "Enable DMA map APIs" ON)
option(BHARAT_ENABLE_COW "Enable Copy-On-Write" ON)
option(BHARAT_ENABLE_DEMAND_PAGING "Enable demand paging" ON)
option(BHARAT_ENABLE_SHARED_MEMORY "Enable shared memory" ON)
option(BHARAT_ENABLE_NUMA "Enable NUMA support" OFF)
option(BHARAT_ENABLE_ALLOC_CLASSES "Enable memory allocation classes" ON)
option(BHARAT_ENABLE_MEMORY_STATS "Enable memory statistics" ON)

# Platform Firmware
set(BHARAT_PLATFORM_FIRMWARE "" CACHE STRING "Target platform firmware (e.g., ACPI, FDT)")

# Boot Hardware Profile
set(BHARAT_BOOT_HW_PROFILE "generic" CACHE STRING "Boot hardware profile (generic, desktop, server, vm, laptop, edge, rtos, automotive_ecu, automotive_infotainment)")
set_property(CACHE BHARAT_BOOT_HW_PROFILE PROPERTY STRINGS generic desktop server vm laptop edge rtos automotive_ecu automotive_infotainment)

# IRQ dispatch profile
set(BHARAT_IRQ_DISPATCH_MODE "AUTO" CACHE STRING "IRQ dispatch mode (AUTO, GENERIC, MIXED, RT)")
set_property(CACHE BHARAT_IRQ_DISPATCH_MODE PROPERTY STRINGS AUTO GENERIC MIXED RT)

# Serial Console Configuration
set(BHARAT_ARCH_SERIAL_BASE "" CACHE STRING "Override default architecture serial base address")
if(BHARAT_ARCH_SERIAL_BASE)
    add_compile_definitions(BHARAT_ARCH_SERIAL_BASE=${BHARAT_ARCH_SERIAL_BASE})
endif()


# Profile-based Memory Capability Overrides
if(BHARAT_DEVICE_PROFILE STREQUAL "RTOS" OR BHARAT_DEVICE_PROFILE STREQUAL "DRONE" OR BHARAT_DEVICE_PROFILE STREQUAL "ROBOT")
    # Tiny MPU/MMU-lite default profiles
    set(BHARAT_ENABLE_ADVANCED_VM OFF CACHE BOOL "Enable advanced virtual memory" FORCE)
    set(BHARAT_ENABLE_COW OFF CACHE BOOL "Enable Copy-On-Write" FORCE)
    set(BHARAT_ENABLE_DEMAND_PAGING OFF CACHE BOOL "Enable demand paging" FORCE)
    set(BHARAT_ENABLE_SHARED_MEMORY OFF CACHE BOOL "Enable shared memory" FORCE)
    set(BHARAT_ENABLE_NUMA OFF CACHE BOOL "Enable NUMA support" FORCE)
    set(BHARAT_ENABLE_MEMORY_STATS OFF CACHE BOOL "Enable memory statistics" FORCE)
    set(BHARAT_PROFILE_MMU_FULL OFF CACHE BOOL "Full VM" FORCE)
    if(ARCH STREQUAL "arm32" OR ARCH STREQUAL "riscv32")
        set(BHARAT_ENABLE_MMU OFF CACHE BOOL "Enable MMU support" FORCE)
        set(BHARAT_ENABLE_MPU ON CACHE BOOL "Enable MPU support" FORCE)
        set(BHARAT_PROFILE_MPU_ONLY ON CACHE BOOL "MPU regions only" FORCE)
        set(BHARAT_PROFILE_MMU_LITE OFF CACHE BOOL "MMU Lite" FORCE)
    else()
        set(BHARAT_ENABLE_MMU ON CACHE BOOL "Enable MMU support" FORCE)
        set(BHARAT_ENABLE_MPU OFF CACHE BOOL "Enable MPU support" FORCE)
        set(BHARAT_PROFILE_MMU_LITE ON CACHE BOOL "MMU Lite" FORCE)
        set(BHARAT_PROFILE_MPU_ONLY OFF CACHE BOOL "MPU regions only" FORCE)
    endif()
elseif(BHARAT_DEVICE_PROFILE STREQUAL "DATACENTER" OR BHARAT_DEVICE_PROFILE STREQUAL "CLOUD" OR BHARAT_DEVICE_PROFILE STREQUAL "SERVER")
    set(BHARAT_ENABLE_NUMA ON CACHE BOOL "Enable NUMA support" FORCE)
    set(BHARAT_ENABLE_IOMMU ON CACHE BOOL "Enable IOMMU support" FORCE)
elseif(BHARAT_DEVICE_PROFILE STREQUAL "DESKTOP" OR BHARAT_DEVICE_PROFILE STREQUAL "LAPTOP" OR BHARAT_DEVICE_PROFILE STREQUAL "AUTOMOTIVE_INFOTAINMENT")
    set(BHARAT_ENABLE_IOMMU ON CACHE BOOL "Enable IOMMU support" FORCE)
endif()

# Determine Kernel Execution Profile (RT, GP, MIX) based on Device Profile
if(BHARAT_DEVICE_PROFILE STREQUAL "RTOS"
   OR BHARAT_DEVICE_PROFILE STREQUAL "DRONE"
   OR BHARAT_DEVICE_PROFILE STREQUAL "ROBOT"
   OR BHARAT_DEVICE_PROFILE STREQUAL "AUTOMOTIVE_ECU")
    set(BHARAT_KERNEL_PROFILE_RESOLVED "RT")
elseif(BHARAT_DEVICE_PROFILE STREQUAL "EDGE"
       OR BHARAT_DEVICE_PROFILE STREQUAL "MOBILE"
       OR BHARAT_DEVICE_PROFILE STREQUAL "AUTOMOTIVE_INFOTAINMENT")
    set(BHARAT_KERNEL_PROFILE_RESOLVED "MIX")
else()
    set(BHARAT_KERNEL_PROFILE_RESOLVED "GP")
endif()
message(STATUS "Configuring for Kernel Execution Profile: ${BHARAT_KERNEL_PROFILE_RESOLVED}")
message(STATUS "Configuring for Device Profile: ${BHARAT_DEVICE_PROFILE}")
message(STATUS "Configuring for Personality Profile: ${BHARAT_PERSONALITY_PROFILE}")
message(STATUS "Architecture: ${BHARAT_ARCH_FAMILY}-${BHARAT_ARCH_BITS} (${BHARAT_ARCH_VARIANT})")

# Normalize user-provided profile/personality selectors
string(TOUPPER "${BHARAT_DEVICE_PROFILE}" BHARAT_DEVICE_PROFILE)
set(BHARAT_DEVICE_PROFILE "${BHARAT_DEVICE_PROFILE}" CACHE STRING "Target device profile (DESKTOP, MOBILE, EDGE, DATACENTER, NETWORK_APPLIANCE, DRONE, ROBOT, RTOS, AUTOMOTIVE_ECU, AUTOMOTIVE_INFOTAINMENT)" FORCE)
string(TOUPPER "${BHARAT_PERSONALITY_PROFILE}" BHARAT_PERSONALITY_PROFILE)
set(BHARAT_PERSONALITY_PROFILE "${BHARAT_PERSONALITY_PROFILE}" CACHE STRING "Target personality profile (NATIVE, LINUX, ANDROID, WINDOWS, MAC)" FORCE)

# Resolve required components for target profile/personality/board
bharat_apply_component_policy()

option(BHARAT_USE_LLD "Prefer LLVM lld linker for Clang builds" ON)

if(CMAKE_C_COMPILER_ID MATCHES "Clang" AND BHARAT_USE_LLD)
    add_link_options(-fuse-ld=lld)
endif()



option(BHARAT_ENABLE_CLANG_TIDY "Enable clang-tidy static analysis for C targets" OFF)
option(BHARAT_BUILD_HOST_TESTS "Build host-side unit tests" OFF)

if(BHARAT_ENABLE_CLANG_TIDY)
    find_program(BHARAT_CLANG_TIDY NAMES clang-tidy)
    if(BHARAT_CLANG_TIDY)
        set(CMAKE_C_CLANG_TIDY "${BHARAT_CLANG_TIDY}")
        message(STATUS "clang-tidy enabled: ${BHARAT_CLANG_TIDY}")
    else()
        message(WARNING "BHARAT_ENABLE_CLANG_TIDY=ON but clang-tidy not found")
    endif()
endif()

# --- Helper logic for config header ---

# Architecture Family
bharat_set_profile_flag(BHARAT_ARCH_X86 "X86" "${BHARAT_ARCH_FAMILY}")
bharat_set_profile_flag(BHARAT_ARCH_ARM "ARM" "${BHARAT_ARCH_FAMILY}")
bharat_set_profile_flag(BHARAT_ARCH_RISCV "RISCV" "${BHARAT_ARCH_FAMILY}")

# Architecture Bitness
bharat_set_profile_flag(BHARAT_ARCH_32BIT "32" "${BHARAT_ARCH_BITS}")
bharat_set_profile_flag(BHARAT_ARCH_64BIT "64" "${BHARAT_ARCH_BITS}")

# Architecture Variant
bharat_set_profile_flag(BHARAT_ARCH_VARIANT_GENERIC "GENERIC" "${BHARAT_ARCH_VARIANT}")
bharat_set_profile_flag(BHARAT_ARCH_VARIANT_SHAKTI "SHAKTI" "${BHARAT_ARCH_VARIANT}")

# Device Profile
bharat_set_profile_flag(BHARAT_PROFILE_DESKTOP "DESKTOP" "${BHARAT_DEVICE_PROFILE}")
bharat_set_profile_flag(BHARAT_PROFILE_MOBILE "MOBILE" "${BHARAT_DEVICE_PROFILE}")
bharat_set_profile_flag(BHARAT_PROFILE_EDGE "EDGE" "${BHARAT_DEVICE_PROFILE}")
bharat_set_profile_flag(BHARAT_PROFILE_DATACENTER "DATACENTER" "${BHARAT_DEVICE_PROFILE}")
bharat_set_profile_flag(BHARAT_PROFILE_NETWORK_APPLIANCE "NETWORK_APPLIANCE" "${BHARAT_DEVICE_PROFILE}")
bharat_set_profile_flag(BHARAT_PROFILE_DRONE "DRONE" "${BHARAT_DEVICE_PROFILE}")
bharat_set_profile_flag(BHARAT_PROFILE_ROBOT "ROBOT" "${BHARAT_DEVICE_PROFILE}")
bharat_set_profile_flag(BHARAT_PROFILE_RTOS "RTOS" "${BHARAT_DEVICE_PROFILE}")
bharat_set_profile_flag(BHARAT_PROFILE_AUTOMOTIVE_ECU "AUTOMOTIVE_ECU" "${BHARAT_DEVICE_PROFILE}")
bharat_set_profile_flag(BHARAT_PROFILE_AUTOMOTIVE_INFOTAINMENT "AUTOMOTIVE_INFOTAINMENT" "${BHARAT_DEVICE_PROFILE}")


# Kernel Execution Profile (RT, GP, MIX)
bharat_set_profile_flag(BHARAT_KERNEL_PROFILE_RT "RT" "${BHARAT_KERNEL_PROFILE_RESOLVED}")
bharat_set_profile_flag(BHARAT_KERNEL_PROFILE_GP "GP" "${BHARAT_KERNEL_PROFILE_RESOLVED}")
bharat_set_profile_flag(BHARAT_KERNEL_PROFILE_MIX "MIX" "${BHARAT_KERNEL_PROFILE_RESOLVED}")

# IRQ dispatch mode (profile-aware default with explicit override)
if(BHARAT_IRQ_DISPATCH_MODE STREQUAL "AUTO")
    if(BHARAT_DEVICE_PROFILE STREQUAL "RTOS"
       OR BHARAT_DEVICE_PROFILE STREQUAL "DRONE"
       OR BHARAT_DEVICE_PROFILE STREQUAL "ROBOT"
       OR BHARAT_DEVICE_PROFILE STREQUAL "AUTOMOTIVE_ECU")
        set(BHARAT_IRQ_DISPATCH_RESOLVED "RT")
    elseif(BHARAT_DEVICE_PROFILE STREQUAL "EDGE"
           OR BHARAT_DEVICE_PROFILE STREQUAL "MOBILE"
           OR BHARAT_DEVICE_PROFILE STREQUAL "AUTOMOTIVE_INFOTAINMENT")
        set(BHARAT_IRQ_DISPATCH_RESOLVED "MIXED")
    else()
        set(BHARAT_IRQ_DISPATCH_RESOLVED "GENERIC")
    endif()
else()
    string(TOUPPER "${BHARAT_IRQ_DISPATCH_MODE}" BHARAT_IRQ_DISPATCH_RESOLVED)
endif()
message(STATUS "IRQ dispatch mode: ${BHARAT_IRQ_DISPATCH_RESOLVED}")
bharat_set_profile_flag(BHARAT_IRQ_DISPATCH_GENERIC "GENERIC" "${BHARAT_IRQ_DISPATCH_RESOLVED}")
bharat_set_profile_flag(BHARAT_IRQ_DISPATCH_MIXED "MIXED" "${BHARAT_IRQ_DISPATCH_RESOLVED}")
bharat_set_profile_flag(BHARAT_IRQ_DISPATCH_RT "RT" "${BHARAT_IRQ_DISPATCH_RESOLVED}")

# Personality Profile
bharat_set_profile_flag(BHARAT_PERSONALITY_NATIVE "NATIVE" "${BHARAT_PERSONALITY_PROFILE}")
bharat_set_profile_flag(BHARAT_PERSONALITY_LINUX "LINUX" "${BHARAT_PERSONALITY_PROFILE}")
bharat_set_profile_flag(BHARAT_PERSONALITY_ANDROID "ANDROID" "${BHARAT_PERSONALITY_PROFILE}")
bharat_set_profile_flag(BHARAT_PERSONALITY_WINDOWS "WINDOWS" "${BHARAT_PERSONALITY_PROFILE}")
bharat_set_profile_flag(BHARAT_PERSONALITY_MAC "MAC" "${BHARAT_PERSONALITY_PROFILE}")

# Platform Firmware
bharat_set_profile_flag(BHARAT_PLATFORM_ACPI "ACPI" "${BHARAT_PLATFORM_FIRMWARE}")
bharat_set_profile_flag(BHARAT_PLATFORM_FDT "FDT" "${BHARAT_PLATFORM_FIRMWARE}")

# Process ISA features and pass them as definitions
if(BHARAT_ISA_FEATURES)
    string(TOUPPER "${BHARAT_ISA_FEATURES}" BHARAT_ISA_FEATURES_UPPER)
    # The BHARAT_ISA_FEATURES should be a list like AVX2;FMA;AES
    foreach(FEATURE ${BHARAT_ISA_FEATURES_UPPER})
        add_compile_definitions(BHARAT_ISA_FEATURE_${FEATURE}=1)
    endforeach()
endif()

# Process ISA baseline and pass as definition
if(BHARAT_ISA_BASELINE)
    string(TOUPPER "${BHARAT_ISA_BASELINE}" BHARAT_ISA_BASELINE_UPPER)
    add_compile_definitions(BHARAT_ISA_BASELINE_${BHARAT_ISA_BASELINE_UPPER}=1)
endif()

# Process Accelerators and pass them as definitions
if(BHARAT_ACCELERATOR_CLASSES)
    string(TOUPPER "${BHARAT_ACCELERATOR_CLASSES}" BHARAT_ACCELERATOR_CLASSES_UPPER)
    # The BHARAT_ACCELERATOR_CLASSES should be a list like GPU;NPU
    foreach(ACCEL ${BHARAT_ACCELERATOR_CLASSES_UPPER})
        add_compile_definitions(BHARAT_ACCEL_${ACCEL}=1)
    endforeach()
endif()


# Add subdirectories

add_subdirectory(core/arch)
add_subdirectory(core/boot)
add_subdirectory(core/hal)
add_subdirectory(core/platform)
if(NOT EXISTS "${CMAKE_SOURCE_DIR}/core/kernel/CMakeLists.txt")
    message(FATAL_ERROR "Missing canonical kernel CMake root: ${CMAKE_SOURCE_DIR}/core/kernel")
endif()
add_subdirectory(core/kernel kernel)
add_subdirectory(core/lib)
add_subdirectory(core/drivers)
add_subdirectory(core/services)
set(BHARAT_USER_ROOT_CANDIDATES
    "${CMAKE_SOURCE_DIR}/experience/user"
    "${CMAKE_SOURCE_DIR}/core/user"
)
set(BHARAT_USER_ROOT "")
foreach(_user_root IN LISTS BHARAT_USER_ROOT_CANDIDATES)
    if(EXISTS "${_user_root}/CMakeLists.txt")
        set(BHARAT_USER_ROOT "${_user_root}")
        if(_user_root MATCHES "/user$" AND NOT _user_root MATCHES "/experience/user$")
            message(WARNING "[migration-warning] Using legacy user root: ${_user_root}")
        endif()
        break()
    endif()
endforeach()
if(NOT BHARAT_USER_ROOT)
    message(FATAL_ERROR "No user root found in experience/user or user")
endif()
add_subdirectory("${BHARAT_USER_ROOT}" "${CMAKE_BINARY_DIR}/bharat_user")
add_subdirectory(core/stacks)
add_subdirectory(core/personalities)

if(BHARAT_BUILD_HOST_TESTS)
    set(BHARAT_TESTS_ROOT_CANDIDATES
        "${CMAKE_SOURCE_DIR}/quality/tests"
        "${CMAKE_SOURCE_DIR}/core/tests"
    )
    set(BHARAT_TESTS_ROOT "")
    foreach(_tests_root IN LISTS BHARAT_TESTS_ROOT_CANDIDATES)
        if(EXISTS "${_tests_root}/CMakeLists.txt")
            set(BHARAT_TESTS_ROOT "${_tests_root}")
            if(_tests_root MATCHES "/tests$" AND NOT _tests_root MATCHES "/quality/tests$")
                message(WARNING "[migration-warning] Using legacy tests root: ${_tests_root}")
            endif()
            break()
        endif()
    endforeach()

    if(NOT BHARAT_TESTS_ROOT)
        message(FATAL_ERROR "No tests root found in quality/tests or tests")
    endif()

    add_subdirectory("${BHARAT_TESTS_ROOT}" "${CMAKE_BINARY_DIR}/bharat_tests")
endif()

# Host-test compatibility targets:
# keep ad-hoc `--target <host-test>` commands non-failing when host tests are disabled.
function(bharat_add_host_test_compat_target target_name guidance_message)
    if(NOT TARGET ${target_name})
        add_custom_target(${target_name}
            COMMAND ${CMAKE_COMMAND} -E echo "${guidance_message}"
        )
    endif()
endfunction()

# Trap syscall target + typo alias compatibility.
if(TARGET test_trap_syscall)
    if(NOT TARGET test_trap_syscal)
        add_custom_target(test_trap_syscal DEPENDS test_trap_syscall)
    endif()
else()
    bharat_add_host_test_compat_target(
        test_trap_syscall
        "Host tests disabled. Set -DBHARAT_BUILD_HOST_TESTS=ON to build trap syscall tests."
    )
    if(NOT TARGET test_trap_syscal)
        add_custom_target(test_trap_syscal DEPENDS test_trap_syscall)
    endif()
endif()

# VM aspace host-test compatibility.
bharat_add_host_test_compat_target(
    test_vmm_aspace
    "Host tests disabled. Set -DBHARAT_BUILD_HOST_TESTS=ON to build test_vmm_aspace."
)

# Automotive subsystem host-test compatibility.
bharat_add_host_test_compat_target(
    test_automotive_subsys
    "Host tests disabled. Reconfigure with -DBHARAT_BUILD_HOST_TESTS=ON to build test_automotive_subsys."
)

# PMM and scheduler host-test compatibility for targeted correctness suites.
bharat_add_host_test_compat_target(
    test_pmm_zone_alloc
    "Host tests disabled. Set -DBHARAT_BUILD_HOST_TESTS=ON to build test_pmm_zone_alloc."
)
bharat_add_host_test_compat_target(
    test_pmm_refpin
    "Host tests disabled. Set -DBHARAT_BUILD_HOST_TESTS=ON to build test_pmm_refpin."
)
bharat_add_host_test_compat_target(
    test_sched_reap
    "Host tests disabled. Set -DBHARAT_BUILD_HOST_TESTS=ON to build test_sched_reap."
)

# Image generation
include(BharatImage)
