cmake_minimum_required(VERSION 3.20)

project(devmgr C ASM)

# Add source files
set(SOURCES main.c)

# Create executable
add_executable(devmgr ${SOURCES})
    target_link_libraries(devmgr PRIVATE bharat_user_buildopts bharat_freestanding)

# Target include directories
target_include_directories(devmgr PRIVATE
    ${CMAKE_SOURCE_DIR}/interface/include
)

# Link against common runtime library and IPC library
target_link_libraries(devmgr PRIVATE
    # TODO: Add runtime and ipc libraries when available
)

bharat_configure_userspace_binary(devmgr)
