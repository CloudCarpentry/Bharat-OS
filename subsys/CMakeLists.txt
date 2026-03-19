cmake_minimum_required(VERSION 3.20)
project(BharatOS_Subsys C ASM)

# ------------------------------------------------------------------------------
# Subsystem Core
# ------------------------------------------------------------------------------
add_library(subsys_manager STATIC
    src/manager.c
    src/subsys_test_runner.c
    src/skb/skb.c
)
target_include_directories(subsys_manager PUBLIC include ${CMAKE_SOURCE_DIR}/kernel/include)
target_link_libraries(subsys_manager PUBLIC bharat_user_buildopts bharat_freestanding)

# ------------------------------------------------------------------------------
# Personality Shims
# ------------------------------------------------------------------------------
add_library(subsys_linux_personality STATIC linux/linux_compat.c)
target_include_directories(subsys_linux_personality PUBLIC include ${CMAKE_SOURCE_DIR}/kernel/include)
target_link_libraries(subsys_linux_personality PUBLIC bharat_user_buildopts bharat_freestanding)

add_library(subsys_windows_personality STATIC windows/win_compat.c)
target_include_directories(subsys_windows_personality PUBLIC include ${CMAKE_SOURCE_DIR}/kernel/include)
target_link_libraries(subsys_windows_personality PUBLIC bharat_user_buildopts bharat_freestanding)

add_library(subsys_automotive STATIC automotive/automotive.c)
target_include_directories(subsys_automotive PUBLIC include ${CMAKE_SOURCE_DIR}/kernel/include)
target_link_libraries(subsys_automotive PUBLIC bharat_user_buildopts bharat_freestanding)

add_library(subsys_android STATIC
    android/android_personality.c
    android/android_process.c
    android/android_boot.c
    android/android_security.c
    android/android_binder_dist.c
)
target_include_directories(subsys_android PUBLIC include ${CMAKE_SOURCE_DIR}/kernel/include)
target_link_libraries(subsys_android PUBLIC bharat_user_buildopts bharat_freestanding)

# ------------------------------------------------------------------------------
# Daemons / Executables
# ------------------------------------------------------------------------------
option(BHARAT_BUILD_AI_GOVERNOR "Build ai_governor user-space demo executable" ON)
if(BHARAT_BUILD_AI_GOVERNOR)
    add_executable(ai_governor
        src/ai_governor.c
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
endif()

if(BHARAT_BUILD_NETWORK_STUBS)
    add_subdirectory(network)
endif()
