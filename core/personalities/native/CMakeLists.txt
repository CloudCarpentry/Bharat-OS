cmake_minimum_required(VERSION 3.20)
project(BharatOS_PersonalityNative C ASM)

# The Native personality implementation
add_library(bharatos_personality_native OBJECT
    personality_native.c
    native_syscall.c
)

target_include_directories(bharatos_personality_native
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/core/kernel/include
        ${CMAKE_SOURCE_DIR}/interface/include
        ${CMAKE_SOURCE_DIR}/core/lib/include
)

target_link_libraries(bharatos_personality_native PUBLIC bharat_freestanding bharat_user_buildopts bharat_generated_includes)
