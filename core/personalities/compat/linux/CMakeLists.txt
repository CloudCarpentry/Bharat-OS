cmake_minimum_required(VERSION 3.20)

add_library(bharatos_personality_linux STATIC
    linux_personality.c
    linux_syscall.c
)

target_include_directories(bharatos_personality_linux
    PUBLIC
        include
        ${CMAKE_SOURCE_DIR}/core/personalities/runtime/include
        ${CMAKE_SOURCE_DIR}/core/personalities/abi/linux/include
        ${CMAKE_SOURCE_DIR}/core/personalities/abi/posix/include
        ${CMAKE_SOURCE_DIR}/core/kernel/include
        ${CMAKE_SOURCE_DIR}/core/lib/include
        ${CMAKE_SOURCE_DIR}/interface/include
)
