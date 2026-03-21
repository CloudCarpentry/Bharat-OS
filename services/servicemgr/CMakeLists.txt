cmake_minimum_required(VERSION 3.20)

project(servicemgr C ASM)

# Add source files
set(SOURCES main.c)

# Create executable
add_executable(servicemgr ${SOURCES})
    target_link_libraries(servicemgr PRIVATE bharat_user_buildopts bharat_freestanding)

# Target include directories
target_include_directories(servicemgr PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# Link against common runtime library and IPC library
target_link_libraries(servicemgr PRIVATE
    # TODO: Add runtime and ipc libraries when available
)

bharat_configure_userspace_binary(servicemgr)
