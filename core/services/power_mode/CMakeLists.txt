cmake_minimum_required(VERSION 3.20)

if (BHARAT_AUTOMOTIVE_PROFILE)
    bharat_configure_userspace_binary(
        TARGET power_mode_service
        SOURCES
            main.c
            state_machine.c
            clients.c
            policy.c
            ../common/thermal_policy.c
        LIBS
            bharat_libruntime
            bharat_libipc
            bharat_libcap
    )
    target_include_directories(power_mode_service PRIVATE ${CMAKE_SOURCE_DIR}/core/services/include ${CMAKE_SOURCE_DIR}/interface/include)
endif()
