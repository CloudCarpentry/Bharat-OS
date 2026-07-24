cmake_minimum_required(VERSION 3.20)
project(BharatOS_Console_Service C ASM)

add_executable(console main.c)
    target_link_libraries(console PRIVATE bharat_user_buildopts bharat_freestanding)

target_include_directories(console PRIVATE
    ../../kernel/include
    ../../services/core/subsysmgr/include
    ../../lib/bharat_libtransport
    ${CMAKE_SOURCE_DIR}/interface/include
    ${CMAKE_SOURCE_DIR}/core/lib/ipc/include
    ${CMAKE_SOURCE_DIR}/core/lib/cap/include
    ${CMAKE_SOURCE_DIR}/core/lib/runtime/include
)



# Link against the subsystem manager and URPC libraries
target_link_libraries(console PRIVATE bharat_libtransport subsys_manager bharat_libipc bharat_libcap bharat_libruntime)

bharat_configure_userspace_binary(console)
