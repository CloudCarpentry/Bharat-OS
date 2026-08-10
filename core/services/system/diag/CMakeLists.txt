# SPDX-License-Identifier: MIT
cmake_minimum_required(VERSION 3.20)

if (BHARAT_AUTOMOTIVE_PROFILE)
    bharat_configure_userspace_binary(
        TARGET diag_service
        SOURCES
            main.c
            store.c
            debounce.c
            freeze_frame.c
            telemetry.c
            src/diag_collector.c
            src/diag_jsonl_writer.c
        LIBS
            bharat_libruntime
            bharat_libipc
            bharat_libcap
    )
    target_include_directories(diag_service PRIVATE include ${CMAKE_SOURCE_DIR}/core/lib/diag/include ${CMAKE_SOURCE_DIR}/core/services/include ${CMAKE_SOURCE_DIR}/interface/uapi ${CMAKE_SOURCE_DIR}/interface/uapi/bharat)
    target_link_libraries(diag_service PRIVATE bharat_diag)
endif()
