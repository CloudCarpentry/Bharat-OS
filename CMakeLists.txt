cmake_minimum_required(VERSION 3.20)
project(BharatOS VERSION 0.8.0 LANGUAGES C ASM)

enable_testing()

# Make the modules directory visible to CMake
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules")
include_directories(${CMAKE_SOURCE_DIR}/kernel/src)
include_directories(${CMAKE_SOURCE_DIR}/kernel/include)

# Load Common Build Policies
include(BharatCommon)
include(BharatProfiles)
include(BharatPersonality)
include(BharatComponentPolicy)

# Versioning Identity System: Generate and load from manifest
set(BHARAT_MANIFEST_JSON "${CMAKE_SOURCE_DIR}/release/manifest/os-release.json")
set(BHARAT_GEN_SCRIPT "${CMAKE_SOURCE_DIR}/tools/gen_manifest.py")
set(BHARAT_GEN_DIR "${CMAKE_BINARY_DIR}/generated")
set(BHARAT_GEN_INCLUDE_DIR "${BHARAT_GEN_DIR}/include")

file(MAKE_DIRECTORY "${BHARAT_GEN_DIR}")
file(MAKE_DIRECTORY "${BHARAT_GEN_INCLUDE_DIR}/bharat")

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
include_directories("${BHARAT_GEN_INCLUDE_DIR}")

# Architecture selection
bharat_set_arch_defaults()

# Device Profile
set(BHARAT_DEVICE_PROFILE "DESKTOP" CACHE STRING "Target device profile (DESKTOP, MOBILE, EDGE, DATACENTER, NETWORK_APPLIANCE, DRONE, ROBOT, RTOS, AUTOMOTIVE_ECU, AUTOMOTIVE_INFOTAINMENT)")
set_property(CACHE BHARAT_DEVICE_PROFILE PROPERTY STRINGS DESKTOP MOBILE EDGE DATACENTER NETWORK_APPLIANCE DRONE ROBOT RTOS AUTOMOTIVE_ECU AUTOMOTIVE_INFOTAINMENT)

# Personality Profile
set(BHARAT_PERSONALITY_PROFILE "NONE" CACHE STRING "Target personality profile (NONE, LINUX, WINDOWS, MAC)")
set_property(CACHE BHARAT_PERSONALITY_PROFILE PROPERTY STRINGS NONE LINUX WINDOWS MAC)

# Board selection metadata (provided by build scripts or manual configure)
set(BHARAT_TARGET_BOARD "" CACHE STRING "Target board identifier (e.g., qemu-virt-riscv64, shakti-c)")

# ISA Extensions
set(BHARAT_ISA_BASELINE "" CACHE STRING "Target ISA baseline (e.g., x86_64_v2, rv64gc)")
set(BHARAT_ISA_FEATURES "" CACHE STRING "Target ISA features as a semicolon-separated list (e.g., avx2;fma;aes or v;zba;zbb)")

# Hardware Accelerators
set(BHARAT_ACCELERATOR_CLASSES "" CACHE STRING "Target accelerator classes as a semicolon-separated list (e.g., gpu;npu;dsp;crypto;vpu)")

# Platform Firmware
set(BHARAT_PLATFORM_FIRMWARE "" CACHE STRING "Target platform firmware (e.g., ACPI, FDT)")

# Boot Hardware Profile
set(BHARAT_BOOT_HW_PROFILE "generic" CACHE STRING "Boot hardware profile (generic, desktop, server, vm, laptop, edge, rtos, automotive_ecu, automotive_infotainment)")
set_property(CACHE BHARAT_BOOT_HW_PROFILE PROPERTY STRINGS generic desktop server vm laptop edge rtos automotive_ecu automotive_infotainment)

message(STATUS "Configuring for Device Profile: ${BHARAT_DEVICE_PROFILE}")
message(STATUS "Configuring for Personality Profile: ${BHARAT_PERSONALITY_PROFILE}")
message(STATUS "Architecture: ${BHARAT_ARCH_FAMILY}-${BHARAT_ARCH_BITS} (${BHARAT_ARCH_VARIANT})")

# Normalize user-provided profile/personality selectors
string(TOUPPER "${BHARAT_DEVICE_PROFILE}" BHARAT_DEVICE_PROFILE)
set(BHARAT_DEVICE_PROFILE "${BHARAT_DEVICE_PROFILE}" CACHE STRING "Target device profile (DESKTOP, MOBILE, EDGE, DATACENTER, NETWORK_APPLIANCE, DRONE, ROBOT, RTOS, AUTOMOTIVE_ECU, AUTOMOTIVE_INFOTAINMENT)" FORCE)
string(TOUPPER "${BHARAT_PERSONALITY_PROFILE}" BHARAT_PERSONALITY_PROFILE)
set(BHARAT_PERSONALITY_PROFILE "${BHARAT_PERSONALITY_PROFILE}" CACHE STRING "Target personality profile (NONE, LINUX, WINDOWS, MAC)" FORCE)

# Resolve required components for target profile/personality/board
bharat_apply_component_policy()

# On Windows with LLVM/Clang, explicitly use lld to avoid invoking missing 'ld' or 'gcc'
if(WIN32)
    add_link_options(-fuse-ld=lld)
endif()

if(ARCH STREQUAL "riscv64")
    add_compile_options(
        $<$<COMPILE_LANGUAGE:C>:-march=rv64g>
        $<$<COMPILE_LANGUAGE:C>:-mabi=lp64d>
        $<$<COMPILE_LANGUAGE:C>:-mcmodel=medany>
        $<$<COMPILE_LANGUAGE:ASM>:-march=rv64g>
        $<$<COMPILE_LANGUAGE:ASM>:-mabi=lp64d>
    )
    add_link_options(-fuse-ld=lld)
elseif(ARCH STREQUAL "riscv32")
    add_compile_options(
        $<$<COMPILE_LANGUAGE:C>:-march=rv32g>
        $<$<COMPILE_LANGUAGE:C>:-mabi=ilp32d>
        $<$<COMPILE_LANGUAGE:C>:-mcmodel=medany>
        $<$<COMPILE_LANGUAGE:ASM>:-march=rv32g>
        $<$<COMPILE_LANGUAGE:ASM>:-mabi=ilp32d>
    )
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

# Personality Profile
bharat_set_profile_flag(BHARAT_PERSONALITY_NONE "NONE" "${BHARAT_PERSONALITY_PROFILE}")
bharat_set_profile_flag(BHARAT_PERSONALITY_LINUX "LINUX" "${BHARAT_PERSONALITY_PROFILE}")
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
add_subdirectory(hal)
add_subdirectory(kernel)
add_subdirectory(lib)
add_subdirectory(drivers)
add_subdirectory(subsys)
add_subdirectory(services)
add_subdirectory(apps)

if(BHARAT_BUILD_HOST_TESTS AND NOT CMAKE_CROSSCOMPILING)
    add_subdirectory(tests)
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

# Image generation
include(BharatImage)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-pie -fno-PIE -mcmodel=large")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pie")
