cmake_minimum_required(VERSION 3.20)

project(coremgr C ASM)

# Add source files
set(SOURCES src/main.c src/registry.c src/lifecycle.c src/diagnostics.c)

# Create executable
add_executable(coremgr ${SOURCES})
    target_link_libraries(coremgr PRIVATE bharat_user_buildopts bharat_freestanding)

# Target include directories
target_include_directories(coremgr PRIVATE
    ${CMAKE_SOURCE_DIR}/interface/include
    include
)

# Link against common runtime library and IPC library (assuming lib/runtime, lib/ipc)
target_link_libraries(coremgr PRIVATE
    bharat_libruntime
    bharatc
)

bharat_configure_userspace_binary(coremgr)

if(NOT CMAKE_CROSSCOMPILING)
    add_executable(test_coremgr tests/test_coremgr.c src/registry.c src/lifecycle.c src/diagnostics.c src/event_stub.c)
    target_compile_definitions(test_coremgr PRIVATE BHARAT_SERVICE_HOST_TEST=1)
    target_include_directories(test_coremgr PRIVATE include ${CMAKE_SOURCE_DIR}/interface/include)
    add_test(NAME test_coremgr COMMAND test_coremgr)
endif()
