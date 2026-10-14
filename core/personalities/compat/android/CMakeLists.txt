cmake_minimum_required(VERSION 3.20)

add_library(bharatos_personality_android STATIC
    android_personality.c
    android_binder_dist.c
    android_security.c
    android_process.c
    android_boot.c
    android_syscall.c
)

target_include_directories(bharatos_personality_android
    PUBLIC
        include
        ${CMAKE_SOURCE_DIR}/core/personalities/runtime/include
        ${CMAKE_SOURCE_DIR}/core/personalities/compat/linux/include
        ${CMAKE_SOURCE_DIR}/core/personalities/abi/linux/include
        ${CMAKE_SOURCE_DIR}/core/kernel/include
        ${CMAKE_SOURCE_DIR}/core/lib/include
        ${CMAKE_SOURCE_DIR}/core/services/core/subsysmgr/include
        ${CMAKE_SOURCE_DIR}/interface/include
)

target_link_libraries(bharatos_personality_android PUBLIC bharat_generated_includes)
