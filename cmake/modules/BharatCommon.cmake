# BharatCommon.cmake
# Common build utilities and helper functions for Bharat-OS

# bharat_set_profile_flag(VAR_NAME MATCH_VALUE ACTUAL_VALUE)
# Sets VAR_NAME to 1 if MATCH_VALUE equals ACTUAL_VALUE (case-insensitive), else 0
function(bharat_set_profile_flag VAR_NAME MATCH_VALUE ACTUAL_VALUE)
    string(TOUPPER "${MATCH_VALUE}" MATCH_UPPER)
    string(TOUPPER "${ACTUAL_VALUE}" ACTUAL_UPPER)
    
    if("${MATCH_UPPER}" STREQUAL "${ACTUAL_UPPER}")
        set(${VAR_NAME} 1 PARENT_SCOPE)
        add_compile_definitions(${VAR_NAME}=1)
    else()
        set(${VAR_NAME} 0 PARENT_SCOPE)
    endif()
endfunction()

# bharat_set_arch_defaults()
# Determines architecture family, bits, and variant from CMAKE_SYSTEM_PROCESSOR
function(bharat_set_arch_defaults)
    set(ARCH_FAMILY "")
    set(ARCH_BITS "")
    set(ARCH_VARIANT "GENERIC")

    set(BHARAT_ARCH_DETECT_CANDIDATE "${CMAKE_SYSTEM_PROCESSOR}")
    if("${BHARAT_ARCH_DETECT_CANDIDATE}" STREQUAL "")
        set(BHARAT_ARCH_DETECT_CANDIDATE "${CMAKE_HOST_SYSTEM_PROCESSOR}")
    endif()
    if("${BHARAT_ARCH_DETECT_CANDIDATE}" STREQUAL "")
        execute_process(
            COMMAND uname -m
            OUTPUT_VARIABLE BHARAT_UNAME_MACHINE
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
        )
        set(BHARAT_ARCH_DETECT_CANDIDATE "${BHARAT_UNAME_MACHINE}")
    endif()

    string(TOLOWER "${BHARAT_ARCH_DETECT_CANDIDATE}" PROC_LOWER)
    
    # Determine architecture family and bitness
    if(PROC_LOWER MATCHES "^(x86_64|amd64|x64)$")
        set(ARCH_FAMILY "X86")
        set(ARCH_BITS "64")
    elseif(PROC_LOWER MATCHES "^(i[3-6]86|x86)$")
        set(ARCH_FAMILY "X86")
        set(ARCH_BITS "32")
    elseif(PROC_LOWER MATCHES "^(aarch64|arm64)$")
        set(ARCH_FAMILY "ARM")
        set(ARCH_BITS "64")
    elseif(PROC_LOWER MATCHES "^(arm|armv[0-9])")
        set(ARCH_FAMILY "ARM")
        set(ARCH_BITS "32")
    elseif(PROC_LOWER MATCHES "^riscv64$")
        set(ARCH_FAMILY "RISCV")
        set(ARCH_BITS "64")
    elseif(PROC_LOWER MATCHES "^riscv32$")
        set(ARCH_FAMILY "RISCV")
        set(ARCH_BITS "32")
    else()
        message(FATAL_ERROR
            "Unknown architecture: system='${CMAKE_SYSTEM_PROCESSOR}', "
            "host='${CMAKE_HOST_SYSTEM_PROCESSOR}', detected='${BHARAT_ARCH_DETECT_CANDIDATE}'")
    endif()
    
    # Check for SHAKTI variant (RISC-V specific)
    if(ARCH_FAMILY STREQUAL "RISCV" AND DEFINED ENV{BHARAT_SHAKTI_VARIANT})
        set(ARCH_VARIANT "SHAKTI")
    endif()
    
    # Export to parent scope
    set(BHARAT_ARCH_FAMILY "${ARCH_FAMILY}" CACHE STRING "Architecture family (X86, ARM, RISCV)" FORCE)
    set(BHARAT_ARCH_BITS "${ARCH_BITS}" CACHE STRING "Architecture bitness (32, 64)" FORCE)
    set(BHARAT_ARCH_VARIANT "${ARCH_VARIANT}" CACHE STRING "Architecture variant (GENERIC, SHAKTI)" FORCE)
    
    message(STATUS "Architecture detected: ${ARCH_FAMILY}-${ARCH_BITS} (${ARCH_VARIANT})")
endfunction()

# Define interface libraries for build options
# These are used throughout the build system to apply consistent flags

# bharat_freestanding: Common freestanding environment flags
add_library(bharat_freestanding INTERFACE)
target_compile_options(bharat_freestanding INTERFACE
    -ffreestanding
    -fno-builtin
    -fno-stack-protector
)

target_link_options(bharat_freestanding INTERFACE
    -nostdlib
)



# Function to apply architecture-specific flags to bharat_freestanding
# This MUST be called after bharat_set_arch_defaults()
function(bharat_apply_arch_flags)
    if(BHARAT_ARCH_FAMILY STREQUAL "RISCV")
        if(BHARAT_ARCH_BITS EQUAL 64)
            target_compile_options(bharat_freestanding INTERFACE
                -march=rv64gc -mabi=lp64d -mcmodel=medany
            )
        elseif(BHARAT_ARCH_BITS EQUAL 32)
            target_compile_options(bharat_freestanding INTERFACE
                -march=rv32g -mabi=ilp32d -mcmodel=medany
            )
        endif()
    elseif(BHARAT_ARCH_FAMILY STREQUAL "ARM")
        if(BHARAT_ARCH_BITS EQUAL 64)
            target_compile_options(bharat_freestanding INTERFACE
                -mcmodel=small
            )
        elseif(BHARAT_ARCH_BITS EQUAL 32)
            target_compile_options(bharat_freestanding INTERFACE
                -march=armv7-a -mfloat-abi=soft
            )
        endif()
    elseif(BHARAT_ARCH_FAMILY STREQUAL "X86")
        if(BHARAT_ARCH_BITS EQUAL 64)
            target_compile_options(bharat_freestanding INTERFACE
                -m64 -mno-red-zone
            )
        endif()
    endif()
endfunction()

# bharat_kernel_buildopts: Kernel-specific build options
add_library(bharat_kernel_buildopts INTERFACE)
target_link_libraries(bharat_kernel_buildopts INTERFACE bharat_freestanding)
target_compile_options(bharat_kernel_buildopts INTERFACE
    -fno-strict-aliasing
    -fno-common
    -fno-exceptions
)
target_compile_definitions(bharat_kernel_buildopts INTERFACE
    __KERNEL__=1
)

# bharat_user_buildopts: User-space build options
add_library(bharat_user_buildopts INTERFACE)
target_link_libraries(bharat_user_buildopts INTERFACE bharat_freestanding)
target_compile_definitions(bharat_user_buildopts INTERFACE
    __USER__=1
)

# Implements per-target userspace PIE policy to avoid conflicting compiler/linker flags
option(BHARAT_USERSPACE_PIE "Build user-space services as Position Independent Executables" OFF)

function(bharat_configure_userspace_library TARGET_NAME)
    if(BHARAT_USERSPACE_PIE)
        set_target_properties(${TARGET_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)
        target_compile_options(${TARGET_NAME} PRIVATE -fPIC)
    else()
        set_target_properties(${TARGET_NAME} PROPERTIES POSITION_INDEPENDENT_CODE OFF)
        target_compile_options(${TARGET_NAME} PRIVATE -fno-pie -fno-PIC)
    endif()
endfunction()

function(bharat_configure_userspace_binary TARGET_NAME)
    # Inject the runtime crt0 object directly into the binary
    if (TARGET bharat_crt0)
        target_sources(${TARGET_NAME} PRIVATE $<TARGET_OBJECTS:bharat_crt0>)
        # Make sure every binary implicitly links the syscalls needed by crt0 (bharat_exit)
        target_link_libraries(${TARGET_NAME} PRIVATE bharat_syscall)
    endif()

    if(BHARAT_USERSPACE_PIE)
        set_target_properties(${TARGET_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)
        target_compile_options(${TARGET_NAME} PRIVATE -fPIE)
        target_link_options(${TARGET_NAME} PRIVATE -nostdlib -pie)
    else()
        set_target_properties(${TARGET_NAME} PROPERTIES POSITION_INDEPENDENT_CODE OFF)
        target_compile_options(${TARGET_NAME} PRIVATE -fno-pie -fno-PIC)
        target_link_options(${TARGET_NAME} PRIVATE -nostdlib "LINKER:-no-pie")
    endif()
endfunction()

# Respect caller/toolchain-selected userspace PIE policy instead of forcing it on.
# Default remains OFF unless explicitly enabled via cache/preset.
set(BHARAT_USERSPACE_PIE "${BHARAT_USERSPACE_PIE}" CACHE BOOL "Build user-space services as Position Independent Executables" FORCE)
