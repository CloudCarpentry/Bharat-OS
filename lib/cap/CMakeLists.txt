cmake_minimum_required(VERSION 3.20)
project(BharatOS_Lib_Cap C ASM)

# Add the lib/cap static library
add_library(cap STATIC src/cap.c)

# Set the public include directory
target_include_directories(cap PUBLIC include)

# Keep the build freestanding and warning-clean
target_compile_options(cap PRIVATE -ffreestanding -nostdlib -Wall)
