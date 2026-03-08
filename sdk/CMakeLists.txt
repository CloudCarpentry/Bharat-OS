cmake_minimum_required(VERSION 3.20)
project(BharatSDK C CXX ASM)

set(CMAKE_C_STANDARD 11)

include_directories(include)

add_library(bharat STATIC lib/src/syscalls.c)

# Provide headers publicly
target_include_directories(bharat PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
