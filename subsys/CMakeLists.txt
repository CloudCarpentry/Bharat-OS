cmake_minimum_required(VERSION 3.20)
project(BharatOS_Subsys C ASM)

include_directories(include)

# Subsystem manager daemon/library
set(SUBSYS_SOURCES
    src/manager.c
    src/subsys_test_runner.c
    linux/linux_compat.c
    windows/win_compat.c
    automotive/automotive.c
    android/android_compat.c
    android/binder_compat.c
    android/ashmem_compat.c
    android/android_service_manager.c
)

# AI Governor daemon (Predictive Resource Scheduling)
set(AI_GOVERNOR_SOURCES
    src/ai_governor.c
)

add_library(subsys_manager STATIC ${SUBSYS_SOURCES})
target_compile_options(subsys_manager PRIVATE -ffreestanding -nostdlib -Wall)
target_include_directories(subsys_manager PRIVATE
    ${CMAKE_SOURCE_DIR}/kernel/include
)

if(CMAKE_SYSTEM_PROCESSOR MATCHES "riscv")
    target_compile_options(subsys_manager PRIVATE -mcmodel=medany)
endif()

option(BHARAT_BUILD_AI_GOVERNOR "Build ai_governor user-space demo executable" ON)
if(BHARAT_BUILD_AI_GOVERNOR)
    add_executable(ai_governor ${AI_GOVERNOR_SOURCES} ../kernel/src/ai_sched.c)
    target_link_libraries(ai_governor PRIVATE subsys_manager urpc c_posix_stub)
    target_include_directories(ai_governor PRIVATE
        ${CMAKE_SOURCE_DIR}/kernel/include
        ${CMAKE_SOURCE_DIR}/lib/urpc
        ${CMAKE_SOURCE_DIR}/lib/posix/include
    )
endif()
# Might need host headers if it's considered user-space mock,
# for now compiled natively or with suitable toolchain for user-space
