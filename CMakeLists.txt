cmake_minimum_required(VERSION 3.20)
project(BharatOS C ASM)

# Architecture selection
set(ARCH "x86_64" CACHE STRING "Target architecture (x86_64, riscv, arm64)")

# OS Profile Configuration (Scalability)
set(OS_PROFILE "DESKTOP" CACHE STRING "Target profile (RTOS, EDGE, DESKTOP)")

if(OS_PROFILE STREQUAL "RTOS")
    add_compile_definitions(Profile_RTOS=1)
    message(STATUS "Configuring for RTOS profile. Disabling GUI servers and large subsystem managers.")
elseif(OS_PROFILE STREQUAL "EDGE")
    add_compile_definitions(Profile_EDGE=1)
    message(STATUS "Configuring for EDGE profile. Intermediate footprint.")
else()
    add_compile_definitions(Profile_DESKTOP=1)
    message(STATUS "Configuring for DESKTOP profile. Enabling full subsystem capabilities.")
endif()

# Add subdirectories
add_subdirectory(kernel)
add_subdirectory(lib)
add_subdirectory(drivers)
add_subdirectory(subsys)
