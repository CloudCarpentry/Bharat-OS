cmake_minimum_required(VERSION 3.20)
project(BharatSDK C CXX ASM)

set(CMAKE_C_STANDARD 11)

include_directories(include)

# Allow building with multikernel features / Shakti
if(CMAKE_SYSTEM_PROCESSOR MATCHES "riscv")
    add_library(bharat STATIC lib/src/syscalls.c lib/src/riscv_syscalls.S)
else()
    add_library(bharat STATIC lib/src/syscalls.c)
endif()

# Provide headers publicly
target_include_directories(bharat PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(bharat PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(bharat PRIVATE bharat_user_buildopts bharat_freestanding)
