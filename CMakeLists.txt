cmake_minimum_required(VERSION 3.20)
project(BharatOS C ASM)

enable_testing()

# Architecture selection
set(BHARAT_ARCH_FAMILY "X86" CACHE STRING "Target architecture family (X86, ARM, RISCV)")
set(BHARAT_ARCH_BITS "64" CACHE STRING "Target architecture bitness (32, 64)")
set(BHARAT_ARCH_VARIANT "GENERIC" CACHE STRING "Target architecture variant (GENERIC, SHAKTI, etc.)")

# Device Profile
set(BHARAT_DEVICE_PROFILE "DESKTOP" CACHE STRING "Target device profile (DESKTOP, MOBILE, EDGE, DATACENTER, NETWORK_APPLIANCE, DRONE, ROBOT, RTOS, AUTOMOTIVE_ECU, AUTOMOTIVE_INFOTAINMENT)")

# Personality Profile
set(BHARAT_PERSONALITY_PROFILE "NONE" CACHE STRING "Target personality profile (NONE, LINUX, WINDOWS, MAC)")

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


# For backward compatibility with existing CMake scripts (like tests, toolchains)
if(BHARAT_ARCH_FAMILY STREQUAL "X86" AND BHARAT_ARCH_BITS STREQUAL "64")
    set(ARCH "x86_64")
elseif(BHARAT_ARCH_FAMILY STREQUAL "RISCV")
    if(BHARAT_ARCH_BITS STREQUAL "64")
        set(ARCH "riscv64")
    else()
        set(ARCH "riscv32")
    endif()
elseif(BHARAT_ARCH_FAMILY STREQUAL "ARM")
    if(BHARAT_ARCH_BITS STREQUAL "64")
        set(ARCH "arm64")
    else()
        set(ARCH "arm")
    endif()
endif()

message(STATUS "Configuring for Device Profile: ${BHARAT_DEVICE_PROFILE}")
message(STATUS "Configuring for Personality Profile: ${BHARAT_PERSONALITY_PROFILE}")
message(STATUS "Architecture: ${BHARAT_ARCH_FAMILY}-${BHARAT_ARCH_BITS} (${BHARAT_ARCH_VARIANT})")

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
if(BHARAT_ARCH_FAMILY STREQUAL "X86")
    set(BHARAT_ARCH_X86 1)
elseif(BHARAT_ARCH_FAMILY STREQUAL "ARM")
    set(BHARAT_ARCH_ARM 1)
elseif(BHARAT_ARCH_FAMILY STREQUAL "RISCV")
    set(BHARAT_ARCH_RISCV 1)
endif()

# Architecture Bitness
if(BHARAT_ARCH_BITS STREQUAL "32")
    set(BHARAT_ARCH_32BIT 1)
elseif(BHARAT_ARCH_BITS STREQUAL "64")
    set(BHARAT_ARCH_64BIT 1)
endif()

# Architecture Variant
if(BHARAT_ARCH_VARIANT STREQUAL "GENERIC")
    set(BHARAT_ARCH_VARIANT_GENERIC 1)
elseif(BHARAT_ARCH_VARIANT STREQUAL "SHAKTI")
    set(BHARAT_ARCH_VARIANT_SHAKTI 1)
endif()

# Device Profile
if(BHARAT_DEVICE_PROFILE STREQUAL "DESKTOP")
    set(BHARAT_PROFILE_DESKTOP 1)
elseif(BHARAT_DEVICE_PROFILE STREQUAL "MOBILE")
    set(BHARAT_PROFILE_MOBILE 1)
elseif(BHARAT_DEVICE_PROFILE STREQUAL "EDGE")
    set(BHARAT_PROFILE_EDGE 1)
elseif(BHARAT_DEVICE_PROFILE STREQUAL "DATACENTER")
    set(BHARAT_PROFILE_DATACENTER 1)
elseif(BHARAT_DEVICE_PROFILE STREQUAL "NETWORK_APPLIANCE")
    set(BHARAT_PROFILE_NETWORK_APPLIANCE 1)
elseif(BHARAT_DEVICE_PROFILE STREQUAL "DRONE")
    set(BHARAT_PROFILE_DRONE 1)
elseif(BHARAT_DEVICE_PROFILE STREQUAL "ROBOT")
    set(BHARAT_PROFILE_ROBOT 1)
elseif(BHARAT_DEVICE_PROFILE STREQUAL "RTOS")
    set(BHARAT_PROFILE_RTOS 1)
elseif(BHARAT_DEVICE_PROFILE STREQUAL "AUTOMOTIVE_ECU")
    set(BHARAT_PROFILE_AUTOMOTIVE_ECU 1)
elseif(BHARAT_DEVICE_PROFILE STREQUAL "AUTOMOTIVE_INFOTAINMENT")
    set(BHARAT_PROFILE_AUTOMOTIVE_INFOTAINMENT 1)
endif()

# Personality Profile
if(BHARAT_PERSONALITY_PROFILE STREQUAL "NONE")
    set(BHARAT_PERSONALITY_NONE 1)
elseif(BHARAT_PERSONALITY_PROFILE STREQUAL "LINUX")
    set(BHARAT_PERSONALITY_LINUX 1)
elseif(BHARAT_PERSONALITY_PROFILE STREQUAL "WINDOWS")
    set(BHARAT_PERSONALITY_WINDOWS 1)
elseif(BHARAT_PERSONALITY_PROFILE STREQUAL "MAC")
    set(BHARAT_PERSONALITY_MAC 1)
endif()

# Platform Firmware
if(BHARAT_PLATFORM_FIRMWARE STREQUAL "ACPI")
    set(BHARAT_PLATFORM_ACPI 1)
elseif(BHARAT_PLATFORM_FIRMWARE STREQUAL "FDT")
    set(BHARAT_PLATFORM_FDT 1)
endif()

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
add_subdirectory(kernel)
add_subdirectory(lib)
add_subdirectory(drivers)
add_subdirectory(subsys)

if(BHARAT_BUILD_HOST_TESTS AND NOT CMAKE_CROSSCOMPILING)
    add_subdirectory(tests)
endif()
