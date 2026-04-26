cmake_minimum_required(VERSION 3.20)
project(BharatOS_Personalities C ASM)

add_subdirectory(common)
add_subdirectory(native)

if(BHARAT_ENABLE_SUBSYS_LINUX)
    add_subdirectory(compat/linux)
endif()

if(BHARAT_ENABLE_SUBSYS_WINDOWS)
    add_subdirectory(compat/windows)
endif()

if(BHARAT_ENABLE_SUBSYS_ANDROID)
    add_subdirectory(compat/android)
endif()

if(BHARAT_ENABLE_SUBSYS_AUTOMOTIVE)
    add_subdirectory(domain/automotive)
endif()

add_library(bharatos_personalities INTERFACE)
target_link_libraries(bharatos_personalities INTERFACE
  bharatos_personality_common
  bharatos_personality_native
)

if(BHARAT_ENABLE_SUBSYS_LINUX)
    target_link_libraries(bharatos_personalities INTERFACE bharatos_personality_linux)
endif()

if(BHARAT_ENABLE_SUBSYS_WINDOWS)
    target_link_libraries(bharatos_personalities INTERFACE bharatos_personality_windows)
endif()

if(BHARAT_ENABLE_SUBSYS_ANDROID)
    target_link_libraries(bharatos_personalities INTERFACE bharatos_personality_android)
endif()

if(BHARAT_ENABLE_SUBSYS_AUTOMOTIVE)
    target_link_libraries(bharatos_personalities INTERFACE bharatos_personality_automotive)
endif()
