cmake_minimum_required(VERSION 3.20)
project(BharatOS_Services_Init C ASM)

# Add the executable target for the init service
add_executable(init
    main.c
    sysmgr.c
    init_profile.c
    init_manifest.c
    init_status.c
    init_handoff.c
    init_runtime.c
)
    target_link_libraries(init PRIVATE bharat_user_buildopts bharat_freestanding)

# Link against the foundational Phase 1 user-space libraries
target_link_libraries(init PUBLIC runtime ipc cap)
target_include_directories(init PRIVATE ../../../kernel/include ../../../kernel/include/generated ${CMAKE_BINARY_DIR}/kernel/include/generated)

# Keep the build freestanding and warning-clean

bharat_configure_userspace_binary(init)
