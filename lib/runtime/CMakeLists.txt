cmake_minimum_required(VERSION 3.20)
project(BharatOS_Lib_Runtime C ASM)

# Add the lib/runtime static library
add_library(runtime STATIC src/runtime.c)

# Set the public include directory
target_include_directories(runtime PUBLIC include)

# Runtime depends on cap and ipc headers
target_link_libraries(runtime PUBLIC cap ipc)

# Keep the build freestanding and warning-clean
target_compile_options(runtime PRIVATE -ffreestanding -nostdlib -Wall)
