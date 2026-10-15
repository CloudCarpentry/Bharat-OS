cmake_minimum_required(VERSION 3.20)
project(ecu_cooling_fan C)

add_executable(ecu_cooling_fan main.c ecu_fan.c)
target_compile_features(ecu_cooling_fan PRIVATE c_std_11)
target_include_directories(ecu_cooling_fan PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

if(TARGET bharat_freestanding)
    target_link_libraries(ecu_cooling_fan PRIVATE
        bharat_user_buildopts
        bharat_libcmin
        bharat_libruntime
        bharatc
        bharat_freestanding
    )
endif()

if(COMMAND bharat_configure_userspace_binary)
    bharat_configure_userspace_binary(ecu_cooling_fan)
endif()
