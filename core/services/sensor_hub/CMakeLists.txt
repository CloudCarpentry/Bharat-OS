cmake_minimum_required(VERSION 3.20)

if (BHARAT_AUTOMOTIVE_PROFILE)
    bharat_configure_userspace_binary(
        TARGET sensor_hub_service
        SOURCES
            main.c
            registry.c
            plausibility.c
            freshness.c
        LIBS
            bharat_libruntime
            bharat_libipc
            bharat_libcap
    )
    target_include_directories(sensor_hub_service PRIVATE ${CMAKE_SOURCE_DIR}/core/services/include ${CMAKE_SOURCE_DIR}/core/drivers/include)
endif()
