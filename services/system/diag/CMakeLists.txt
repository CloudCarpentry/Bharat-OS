cmake_minimum_required(VERSION 3.20)

if (BHARAT_AUTOMOTIVE_PROFILE)
    bharat_configure_userspace_binary(
        TARGET diag_service
        SOURCES
            main.c
            store.c
            debounce.c
            freeze_frame.c
        LIBS
            bharat_libruntime
            bharat_libipc
            bharat_libcap
    )
    target_include_directories(diag_service PRIVATE ${CMAKE_SOURCE_DIR}/services/include)
endif()
