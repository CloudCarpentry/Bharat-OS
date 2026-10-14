cmake_minimum_required(VERSION 3.20)

add_library(bharatos_personality_linux STATIC
    linux_personality.c
    linux_syscall.c
    linux_futex.c
    linux_pipe.c
    linux_eventfd.c
    linux_timerfd.c
    linux_epoll.c
    linux_procfs.c
)

target_include_directories(bharatos_personality_linux
    PUBLIC
        include
        ${CMAKE_SOURCE_DIR}/core/personalities/runtime/include
        ${CMAKE_SOURCE_DIR}/core/personalities/abi/linux/include
        ${CMAKE_SOURCE_DIR}/core/personalities/abi/posix/include
        ${CMAKE_SOURCE_DIR}/core/kernel/include
        ${CMAKE_SOURCE_DIR}/core/lib/include
        ${CMAKE_SOURCE_DIR}/core/services/core/subsysmgr/include
        ${CMAKE_SOURCE_DIR}/interface/include
)

target_link_libraries(bharatos_personality_linux PUBLIC bharat_generated_includes)
