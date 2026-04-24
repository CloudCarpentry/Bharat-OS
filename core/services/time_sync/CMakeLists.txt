cmake_minimum_required(VERSION 3.20)

if (BHARAT_AUTOMOTIVE_PROFILE)
    bharat_configure_userspace_binary(
        TARGET time_sync_service
        SOURCES
            main.c
            drift.c
            quality.c
        LIBS
            bharat_libruntime
            bharat_libipc
            bharat_libcap
    )
    target_include_directories(time_sync_service PRIVATE ${CMAKE_SOURCE_DIR}/services/include)
endif()
