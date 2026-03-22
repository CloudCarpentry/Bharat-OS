cmake_minimum_required(VERSION 3.20)
project(BharatOS_Subsys C ASM)

# ------------------------------------------------------------------------------
# Subsystem Core
# ------------------------------------------------------------------------------
add_library(subsys_manager STATIC
    ../services/core/subsysmgr/manager.c
    ../services/core/subsysmgr/subsys_test_runner.c
    ../stacks/network/skb.c
    ../personalities/common/version_registry.c
)
target_include_directories(subsys_manager PUBLIC include ${CMAKE_SOURCE_DIR}/kernel/include ${CMAKE_SOURCE_DIR}/lib/packet/include ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/stacks/network/include)
target_link_libraries(subsys_manager PUBLIC bharat_user_buildopts bharat_freestanding bharat_generated_includes bharat_libpacket)

# ------------------------------------------------------------------------------
# Personality / Domain Shims
# ------------------------------------------------------------------------------
if(BHARAT_ENABLE_SUBSYS_LINUX)
    add_library(subsys_linux_personality STATIC ../personalities/linux/linux_compat.c)
    target_include_directories(subsys_linux_personality PUBLIC include ${CMAKE_SOURCE_DIR}/kernel/include)
    target_link_libraries(subsys_linux_personality PUBLIC bharat_user_buildopts bharat_freestanding)
endif()

if(BHARAT_ENABLE_SUBSYS_WINDOWS)
    add_library(subsys_windows_personality STATIC ../personalities/windows/win_compat.c)
    target_include_directories(subsys_windows_personality PUBLIC include ${CMAKE_SOURCE_DIR}/kernel/include)
    target_link_libraries(subsys_windows_personality PUBLIC bharat_user_buildopts bharat_freestanding)
endif()

if(BHARAT_ENABLE_SUBSYS_AUTOMOTIVE)
    add_library(subsys_automotive STATIC ../personalities/automotive/automotive.c)
    target_include_directories(subsys_automotive PUBLIC include ${CMAKE_SOURCE_DIR}/kernel/include)
    target_link_libraries(subsys_automotive PUBLIC bharat_user_buildopts bharat_freestanding)
endif()

if(BHARAT_ENABLE_SUBSYS_ANDROID)
    add_library(subsys_android STATIC
        ../personalities/android/android_personality.c
        ../personalities/android/android_process.c
        ../personalities/android/android_boot.c
        ../personalities/android/android_security.c
        ../personalities/android/android_binder_dist.c
    )
    target_include_directories(subsys_android PUBLIC include ${CMAKE_SOURCE_DIR}/kernel/include)
    target_link_libraries(subsys_android PUBLIC bharat_user_buildopts bharat_freestanding)
endif()

# ------------------------------------------------------------------------------
# Daemons / Executables
# ------------------------------------------------------------------------------
option(BHARAT_BUILD_AI_GOVERNOR "Build ai_governor user-space demo executable" ON)
if(BHARAT_BUILD_AI_GOVERNOR)
    add_executable(ai_governor
        ../user/apps/ai_governor/ai_governor.c
        ../kernel/src/ai_sched.c
        ../kernel/src/sched_deg.c
        ../kernel/src/sched_stub.c
    )
    target_link_libraries(ai_governor PRIVATE
        subsys_manager
        bharat_libtransport
        bharat_libcmin
        bharat_user_buildopts
        bharat_freestanding
    )
    target_include_directories(ai_governor PRIVATE
        ${CMAKE_SOURCE_DIR}/kernel/include
    )
    bharat_configure_userspace_binary(ai_governor)
endif()

if(BHARAT_BUILD_NETWORK_STUBS AND BHARAT_ENABLE_SERVICE_NETWORK)
    add_subdirectory(network)
endif()
