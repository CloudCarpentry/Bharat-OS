cmake_minimum_required(VERSION 3.20)

# Detect if we are building standalone or embedded inside Bharat-OS
set(BHARATLIBC_IS_TOP_LEVEL OFF)
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(BHARATLIBC_IS_TOP_LEVEL ON)
endif()

if(BHARATLIBC_IS_TOP_LEVEL)
    project(BharatLibC C ASM)
endif()

# Set up module path and include helper files
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(BharatOSProfileBridge)
include(BharatLibCOptions)
include(BharatLibCProfiles)
include(BharatLibCToolchain)

# Enable meta tests option
option(BHARATLIBC_ENABLE_META_TESTS "Enable meta tests (header containment, boundary checks, and relocation)" ON)

# ---------------------------------------------------------
# Configuration Generation
# ---------------------------------------------------------
# Set up default device profile and personality info based on the chosen profile
if(BHARATLIBC_PROFILE STREQUAL "nano_mpu")
    set(DEFAULT_DEVICE_PROFILE "MICRO/RTOS")
    set(DEFAULT_PERSONALITY "NATIVE")
elseif(BHARATLIBC_PROFILE STREQUAL "embedded_mmulite")
    set(DEFAULT_DEVICE_PROFILE "SMALL/EDGE")
    set(DEFAULT_PERSONALITY "NATIVE")
elseif(BHARATLIBC_PROFILE STREQUAL "rt_safe")
    set(DEFAULT_DEVICE_PROFILE "DRONE/ROBOT/AUTOMOTIVE_ECU")
    set(DEFAULT_PERSONALITY "RT_EMBEDDED")
elseif(BHARATLIBC_PROFILE STREQUAL "edge_posix")
    set(DEFAULT_DEVICE_PROFILE "EDGE/NETWORK_APPLIANCE")
    set(DEFAULT_PERSONALITY "POSIX")
elseif(BHARATLIBC_PROFILE STREQUAL "hosted_desktop")
    set(DEFAULT_DEVICE_PROFILE "DESKTOP/DATACENTER")
    set(DEFAULT_PERSONALITY "POSIX")
else()
    set(DEFAULT_DEVICE_PROFILE "HOST_ENV")
    set(DEFAULT_PERSONALITY "HOST_TEST")
endif()

# Map profile variables to config.h preprocessor values
if(BHARATLIBC_ARCH STREQUAL "x86_64")
    set(BH_LIBC_ARCH_X86_64 1)
elseif(BHARATLIBC_ARCH STREQUAL "arm64")
    set(BH_LIBC_ARCH_ARM64 1)
elseif(BHARATLIBC_ARCH STREQUAL "riscv64")
    set(BH_LIBC_ARCH_RISCV64 1)
elseif(BHARATLIBC_ARCH STREQUAL "arm32")
    set(BH_LIBC_ARCH_ARM32 1)
elseif(BHARATLIBC_ARCH STREQUAL "riscv32")
    set(BH_LIBC_ARCH_RISCV32 1)
endif()

if(BHARATLIBC_MEMORY_MODEL STREQUAL "MMU_FULL")
    set(BH_LIBC_MEMORY_MMU_FULL 1)
elseif(BHARATLIBC_MEMORY_MODEL STREQUAL "MMU_LITE")
    set(BH_LIBC_MEMORY_MMU_LITE 1)
elseif(BHARATLIBC_MEMORY_MODEL STREQUAL "MPU")
    set(BH_LIBC_MEMORY_MPU 1)
endif()

if(BHARATLIBC_EXECUTION_PROFILE STREQUAL "GP")
    set(BH_LIBC_EXECUTION_GP 1)
elseif(BHARATLIBC_EXECUTION_PROFILE STREQUAL "RT")
    set(BH_LIBC_EXECUTION_RT 1)
elseif(BHARATLIBC_EXECUTION_PROFILE STREQUAL "MIX")
    set(BH_LIBC_EXECUTION_MIX 1)
endif()

if(BHARATLIBC_PROFILE STREQUAL "nano_mpu")
    set(BH_LIBC_PROFILE_NANO_MPU 1)
elseif(BHARATLIBC_PROFILE STREQUAL "embedded_mmulite")
    set(BH_LIBC_PROFILE_EMBEDDED_MMULITE 1)
elseif(BHARATLIBC_PROFILE STREQUAL "rt_safe")
    set(BH_LIBC_PROFILE_RT_SAFE 1)
elseif(BHARATLIBC_PROFILE STREQUAL "edge_posix")
    set(BH_LIBC_PROFILE_EDGE_POSIX 1)
elseif(BHARATLIBC_PROFILE STREQUAL "hosted_desktop")
    set(BH_LIBC_PROFILE_HOSTED_DESKTOP 1)
elseif(BHARATLIBC_PROFILE STREQUAL "host_test")
    set(BH_LIBC_PROFILE_HOST_TEST 1)
endif()

set(BH_LIBC_HAS_FIXED_ALLOCATOR ${BHARATLIBC_HAS_FIXED_ALLOCATOR})
set(BH_LIBC_HAS_POSIX_IO ${BHARATLIBC_HAS_POSIX_IO})
set(BH_LIBC_HAS_PTHREAD ${BHARATLIBC_HAS_PTHREAD})
set(BH_LIBC_HAS_FLOAT_PRINTF ${BHARATLIBC_HAS_FLOAT_PRINTF})

# Generate headers & share configuration artifacts
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/include/bharat/libc/config.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/include/bharat/libc/config.h"
)

string(TIMESTAMP CURRENT_DATE_STR "%Y-%m-%d")

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/generated/share/bharatlibc/profile.json"
"{
  \"profile\": \"${BHARATLIBC_PROFILE}\",
  \"memory_model\": \"${BHARATLIBC_MEMORY_MODEL}\",
  \"execution_profile\": \"${BHARATLIBC_EXECUTION_PROFILE}\",
  \"device_profile\": \"${DEFAULT_DEVICE_PROFILE}\",
  \"personality\": \"${DEFAULT_PERSONALITY}\"
}\n")

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/generated/share/bharatlibc/build-info.json"
"{
  \"version\": \"1.0.0\",
  \"build_date\": \"${CURRENT_DATE_STR}\",
  \"build_type\": \"${CMAKE_BUILD_TYPE}\",
  \"target_arch\": \"${BHARATLIBC_ARCH}\"
}\n")

# ---------------------------------------------------------
# Build Interface / Install Interface Paths
# ---------------------------------------------------------
set(BUILD_INCLUDES
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/generated/include>
    $<INSTALL_INTERFACE:include>
)

if(BHARATLIBC_UAPI_ROOT)
    list(APPEND BUILD_INCLUDES $<BUILD_INTERFACE:${BHARATLIBC_UAPI_ROOT}>)
endif()

# ---------------------------------------------------------
# Target Definitions
# ---------------------------------------------------------

# 1. bharatlibc_core
add_library(bharatlibc_core STATIC
    src/core/memory/memory.c
    src/core/string/string.c
    src/core/errno/errno.c
    src/core/assert/assert.c
    src/core/build_info/build_info.c
)
target_include_directories(bharatlibc_core PUBLIC ${BUILD_INCLUDES})
set_target_properties(bharatlibc_core PROPERTIES EXPORT_NAME Core)

# Add compiler definitions for build/profile details
target_compile_definitions(bharatlibc_core PRIVATE
    BH_LIBC_VERSION="1.0.0"
    BH_LIBC_BUILD_DATE="${CURRENT_DATE_STR}"
    BH_LIBC_BUILD_TYPE="${CMAKE_BUILD_TYPE}"
    BH_LIBC_TARGET_ARCH="${BHARATLIBC_ARCH}"
    BH_LIBC_PROFILE_NAME="${BHARATLIBC_PROFILE}"
    BH_LIBC_MEMORY_MODEL="${BHARATLIBC_MEMORY_MODEL}"
    BH_LIBC_EXECUTION_PROFILE="${BHARATLIBC_EXECUTION_PROFILE}"
    BH_LIBC_DEVICE_PROFILE="${DEFAULT_DEVICE_PROFILE}"
    BH_LIBC_PERSONALITY="${DEFAULT_PERSONALITY}"
)

if(BHARATLIBC_BACKEND STREQUAL "HOST")
    target_compile_definitions(bharatlibc_core PUBLIC BHARATLIBC_HOST_MODE=1)
endif()

# 2. bharatlibc_bsys
add_library(bharatlibc_bsys STATIC
    src/bsys/backend_dispatch.c
    src/bsys/status_errno.c
    src/bsys/runtime_caps.c
    src/backends/host/host_backend.c
    src/backends/bharat/bharat_backend.c
)
target_include_directories(bharatlibc_bsys PUBLIC ${BUILD_INCLUDES})
target_link_libraries(bharatlibc_bsys PUBLIC bharatlibc_core)
set_target_properties(bharatlibc_bsys PROPERTIES EXPORT_NAME BSys)

if(BHARATLIBC_BACKEND STREQUAL "HOST")
    target_compile_definitions(bharatlibc_bsys PRIVATE BHARATLIBC_HOST_MODE=1)
endif()

# 3. bharatlibc_alloc
add_library(bharatlibc_alloc STATIC
    src/alloc/allocator_dispatch.c
    src/alloc/fixed_arena.c
    src/alloc/bounded_freelist.c
)
target_include_directories(bharatlibc_alloc PUBLIC ${BUILD_INCLUDES})
target_link_libraries(bharatlibc_alloc PUBLIC bharatlibc_core)
set_target_properties(bharatlibc_alloc PROPERTIES EXPORT_NAME Alloc)

# 4. bharatlibc_stdio_min
add_library(bharatlibc_stdio_min STATIC
    src/stdio/integer_output.c
)
target_include_directories(bharatlibc_stdio_min PUBLIC ${BUILD_INCLUDES})
target_link_libraries(bharatlibc_stdio_min PUBLIC bharatlibc_core bharatlibc_bsys)
set_target_properties(bharatlibc_stdio_min PROPERTIES EXPORT_NAME StdioMin)

# 5. bharatlibc_crt (freestanding CRT initialization stub)
add_library(bharatlibc_crt STATIC src/crt/common/crt_stub.c)
target_include_directories(bharatlibc_crt PUBLIC ${BUILD_INCLUDES})
set_target_properties(bharatlibc_crt PROPERTIES EXPORT_NAME CRT)

# Create empty crt_stub.c if not already present
file(WRITE "${CMAKE_CURRENT_SOURCE_DIR}/src/crt/common/crt_stub.c" "/* CRT Stub initialization */\nvoid __bh_libc_crt_init(void) {}\n")

# ---------------------------------------------------------
# Namespaced Aliases
# ---------------------------------------------------------
add_library(BharatLibC::Core ALIAS bharatlibc_core)
add_library(BharatLibC::BSys ALIAS bharatlibc_bsys)
add_library(BharatLibC::Alloc ALIAS bharatlibc_alloc)
add_library(BharatLibC::StdioMin ALIAS bharatlibc_stdio_min)
add_library(BharatLibC::CRT ALIAS bharatlibc_crt)

# ---------------------------------------------------------
# Installation & Packaging
# ---------------------------------------------------------
if(BHARATLIBC_IS_TOP_LEVEL)
    include(BharatLibCInstall)
endif()

# ---------------------------------------------------------
# Unit Testing
# ---------------------------------------------------------
if(BHARATLIBC_BUILD_TESTS)
    if(BHARATLIBC_IS_TOP_LEVEL)
        enable_testing()
    endif()
    add_subdirectory(tests)
endif()
