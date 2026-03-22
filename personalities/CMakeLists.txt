cmake_minimum_required(VERSION 3.20)
project(BharatOS_Personalities C ASM)

add_subdirectory(common)
add_subdirectory(linux)
add_subdirectory(windows)
add_subdirectory(android)
add_subdirectory(automotive)

add_library(bharatos_personalities INTERFACE)
target_link_libraries(bharatos_personalities INTERFACE
  bharatos_personality_common
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
