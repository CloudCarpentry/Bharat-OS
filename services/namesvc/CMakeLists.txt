cmake_minimum_required(VERSION 3.20)
project(BharatOS_Services_NameSvc C ASM)

# Add the executable target for the namesvc daemon
add_executable(namesvc main.c)

# Link against the foundational Phase 1 user-space libraries
target_link_libraries(namesvc PUBLIC runtime ipc cap)

# Keep the build freestanding and warning-clean
target_compile_options(namesvc PRIVATE -ffreestanding -nostdlib -Wall)
