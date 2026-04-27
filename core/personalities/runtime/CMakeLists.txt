cmake_minimum_required(VERSION 3.20)

add_library(bharatos_personality_runtime STATIC
    src/personality_registry.c
    src/translation_events.c
    src/version_registry.c
)

target_include_directories(bharatos_personality_runtime
    PUBLIC
        include
        ${CMAKE_SOURCE_DIR}/core/kernel/include
        ${CMAKE_SOURCE_DIR}/interface/include
)
