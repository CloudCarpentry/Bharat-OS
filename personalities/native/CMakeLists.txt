cmake_minimum_required(VERSION 3.20)
project(BharatOS_PersonalityNative C ASM)

# The Native personality implementation
add_library(bharatos_personality_native OBJECT
    personality_native.c
)

target_include_directories(bharatos_personality_native
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/kernel/include
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/lib/include
)

target_link_libraries(bharatos_personality_native PUBLIC bharat_freestanding bharat_user_buildopts bharat_generated_includes)
