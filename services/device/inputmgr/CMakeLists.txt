cmake_minimum_required(VERSION 3.20)

add_library(inputmgr STATIC
    inputmgr.c
)

target_include_directories(inputmgr PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    ${CMAKE_SOURCE_DIR}/uapi
    ${CMAKE_SOURCE_DIR}/drivers/input/include
)

target_link_libraries(inputmgr PRIVATE bharat_user_buildopts bharat_freestanding)

# For eventual IPC bindings, etc.
# target_link_libraries(inputmgr PRIVATE bharat_libmsg)
