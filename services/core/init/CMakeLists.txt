cmake_minimum_required(VERSION 3.20)
project(BharatOS_Services_Init C ASM)

option(BHARAT_INIT_ENABLE_HANDOFF "Enable handoff to servicemgr" ON)
option(BHARAT_INIT_ENABLE_RETRY "Enable service start retries" ON)
option(BHARAT_INIT_ENABLE_STATUS_QUERY "Enable boot status querying/logging" ON)
option(BHARAT_INIT_ENABLE_OPTIONAL_SERVICES "Enable optional services (ignored if tiny)" ON)

# Add the executable target for the init service
add_executable(init
    init_main.c
    init_manifest.c
    init_profile.c
    init_runtime.c
    init_status.c
    init_handoff.c
)
target_link_libraries(init PRIVATE bharat_user_buildopts bharat_freestanding bharat_syscall)

# Link against the foundational Phase 1 user-space libraries
target_link_libraries(init PUBLIC runtime ipc cap)

# Configure compiler flags based on init options
if(BHARAT_INIT_ENABLE_HANDOFF)
    target_compile_definitions(init PRIVATE BHARAT_INIT_ENABLE_HANDOFF)
endif()
if(BHARAT_INIT_ENABLE_RETRY)
    target_compile_definitions(init PRIVATE BHARAT_INIT_ENABLE_RETRY)
endif()
if(BHARAT_INIT_ENABLE_STATUS_QUERY)
    target_compile_definitions(init PRIVATE BHARAT_INIT_ENABLE_STATUS_QUERY)
endif()

if(DEFINED BHARAT_DEFAULT_INIT_PROFILE AND NOT BHARAT_DEFAULT_INIT_PROFILE STREQUAL "")
    target_compile_definitions(init PRIVATE BHARAT_DEFAULT_INIT_PROFILE="${BHARAT_DEFAULT_INIT_PROFILE}")
    # Also define the specific enum flag for the fallback
    target_compile_definitions(init PRIVATE BHARAT_INIT_PROFILE_${BHARAT_DEFAULT_INIT_PROFILE})
endif()

# Keep the build freestanding and warning-clean
bharat_configure_userspace_binary(init)
