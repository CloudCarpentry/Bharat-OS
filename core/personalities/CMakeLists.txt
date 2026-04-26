cmake_minimum_required(VERSION 3.20)
project(BharatOS_Personalities C ASM)

add_subdirectory(runtime)
add_subdirectory(abi/linux)
add_subdirectory(native)
add_subdirectory(compat/linux)
add_subdirectory(compat/android)
add_subdirectory(compat/windows)
add_subdirectory(domain/automotive)

add_library(bharatos_personalities INTERFACE)

if(BHARAT_ENABLE_PERSONALITIES)
    target_link_libraries(bharatos_personalities INTERFACE
        bharatos_personality_runtime
        bharatos_personality_native
    )
endif()

if(BHARAT_ENABLE_COMPAT_LINUX)
    target_link_libraries(bharatos_personalities INTERFACE bharatos_personality_linux bharatos_personality_abi_linux)
endif()

if(BHARAT_ENABLE_COMPAT_ANDROID)
    target_link_libraries(bharatos_personalities INTERFACE bharatos_personality_android)
endif()

if(BHARAT_ENABLE_SUBSYS_WINDOWS)
    target_link_libraries(bharatos_personalities INTERFACE bharatos_personality_windows)
endif()

if(BHARAT_ENABLE_SUBSYS_AUTOMOTIVE)
    target_link_libraries(bharatos_personalities INTERFACE bharatos_personality_automotive)
endif()
