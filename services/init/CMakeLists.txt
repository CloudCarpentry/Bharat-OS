cmake_minimum_required(VERSION 3.20)
project(BharatOS_Services_Init C ASM)

# Add the executable target for the init service
add_executable(init main.c)
    target_link_libraries(init PRIVATE bharat_user_buildopts bharat_freestanding)

# Link against the foundational Phase 1 user-space libraries
target_link_libraries(init PUBLIC runtime ipc cap)

# Keep the build freestanding and warning-clean

bharat_configure_userspace_binary(init)
