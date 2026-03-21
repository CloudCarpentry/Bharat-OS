cmake_minimum_required(VERSION 3.20)
project(BharatOS_CAN_Service C ASM)

if(COMMAND bharat_component_option)
    bharat_component_option(BHARAT_ENABLE_SERVICE_CAN OFF "Build CAN service")
else()
    option(BHARAT_ENABLE_SERVICE_CAN "Build CAN service" OFF)
endif()

if(BHARAT_ENABLE_SERVICE_CAN)
    set(BHARAT_CAN_SVC_SOURCES
        main.c
    )

    add_executable(can_service ${BHARAT_CAN_SVC_SOURCES})

    target_include_directories(can_service PRIVATE .)
    target_include_directories(can_service PRIVATE ${CMAKE_SOURCE_DIR}/include)
    target_include_directories(can_service PRIVATE ${CMAKE_SOURCE_DIR}/drivers/include)

    target_link_libraries(can_service PRIVATE
        bharat_user_buildopts
        bharat_freestanding
        bharat_libipc
        bharat_libcap
        bharatos_drivers
    )

    bharat_configure_userspace_binary(can_service)
endif()
