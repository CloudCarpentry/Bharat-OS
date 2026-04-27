cmake_minimum_required(VERSION 3.20)

add_library(bharatos_personality_abi_linux STATIC
    src/linux_errno.c
)

target_include_directories(bharatos_personality_abi_linux
    PUBLIC
        include
        ${CMAKE_SOURCE_DIR}/core/personalities/abi/posix/include
        ${CMAKE_SOURCE_DIR}/core/kernel/include
        ${CMAKE_SOURCE_DIR}/interface/include
)
