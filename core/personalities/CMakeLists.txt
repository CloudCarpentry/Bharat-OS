cmake_minimum_required(VERSION 3.20)
project(BharatOS_Personalities C ASM)

add_subdirectory(common)
add_subdirectory(native)
add_subdirectory(compat/linux)
add_subdirectory(compat/windows)
add_subdirectory(compat/android)
add_subdirectory(domain/automotive)

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
