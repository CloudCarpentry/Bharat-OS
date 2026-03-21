cmake_minimum_required(VERSION 3.20)

if (BHARAT_AUTOMOTIVE_PROFILE OR BHARAT_ENABLE_CAN)
    bharat_configure_userspace_binary(
        TARGET can_service
        SOURCES
            main.c
            router.c
            subscriptions.c
            diag.c
            gateway.c
        LIBS
            bharat_libruntime
            bharat_libipc
            bharat_libcap
            bharat_driver_can
    )
    target_include_directories(can_service PRIVATE ${CMAKE_SOURCE_DIR}/services/include)
endif()
