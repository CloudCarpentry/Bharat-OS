cmake_minimum_required(VERSION 3.20)
project(BharatOS_Services_Init C ASM)

# Add the executable target for the init service
add_executable(init main.c)

# Link against the foundational Phase 1 user-space libraries
target_link_libraries(init PUBLIC runtime ipc cap)

# Keep the build freestanding and warning-clean
target_compile_options(init PRIVATE -ffreestanding -nostdlib -Wall)
target_link_options(init PRIVATE "-nostdlib" "-no-pie" "-fno-pie" "-no-pie" "-fno-pie")
