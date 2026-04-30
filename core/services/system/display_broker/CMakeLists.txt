cmake_minimum_required(VERSION 3.20)
project(BharatOS_System_DisplayBroker C)

set(DISPLAY_BROKER_SOURCES
    main.c
)

add_executable(display_broker ${DISPLAY_BROKER_SOURCES})
target_link_libraries(display_broker PRIVATE
    bharat_user_buildopts
    bharat_freestanding
    bharat_libruntime
    bharat_libipc
    bharat_libcap
    bharat_service_runtime
)
target_include_directories(display_broker PRIVATE
    ${CMAKE_SOURCE_DIR}/interface
    ${CMAKE_SOURCE_DIR}/interface/include
    ${CMAKE_SOURCE_DIR}/core/lib/ipc/include
    ${CMAKE_SOURCE_DIR}/core/lib/cap/include
    ${CMAKE_SOURCE_DIR}/core/lib/runtime/include
    ${CMAKE_SOURCE_DIR}/core/services/common/runtime/include
)

bharat_configure_userspace_binary(display_broker)
