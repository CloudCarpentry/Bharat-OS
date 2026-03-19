cmake_minimum_required(VERSION 3.20)
project(BharatOS_Services_NameSvc C ASM)

# Add the executable target for the namesvc daemon
add_executable(namesvc main.c)
    target_link_libraries(namesvc PRIVATE bharat_user_buildopts bharat_freestanding)

# Link against the foundational Phase 1 user-space libraries
target_link_libraries(namesvc PUBLIC runtime ipc cap)

# Keep the build freestanding and warning-clean
