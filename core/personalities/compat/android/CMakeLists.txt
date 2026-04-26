cmake_minimum_required(VERSION 3.20)

add_library(bharatos_personality_android STATIC
    android_personality.c
)

target_include_directories(bharatos_personality_android
    PUBLIC
        include
        ${CMAKE_SOURCE_DIR}/core/personalities/runtime/include
        ${CMAKE_SOURCE_DIR}/core/personalities/compat/linux/include
        ${CMAKE_SOURCE_DIR}/core/kernel/include
        ${CMAKE_SOURCE_DIR}/interface/include
)
