cmake_minimum_required(VERSION 3.20)
project(BharatOS_Lib_IPC C ASM)

# Add the lib/ipc static library
add_library(ipc STATIC src/ipc.c)

# Set the public include directory
target_include_directories(ipc PUBLIC include)

# IPC depends on cap headers
target_link_libraries(ipc PUBLIC cap)

# Keep the build freestanding and warning-clean
target_compile_options(ipc PRIVATE -ffreestanding -nostdlib -Wall)
