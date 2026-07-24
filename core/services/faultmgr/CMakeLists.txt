cmake_minimum_required(VERSION 3.20)

project(faultmgr C ASM)

# Add source files
set(SOURCES main.c)

# Create executable
add_executable(faultmgr ${SOURCES})
    target_link_libraries(faultmgr PRIVATE bharat_user_buildopts bharat_freestanding)

# Target include directories
target_include_directories(faultmgr PRIVATE
    ${CMAKE_SOURCE_DIR}/interface/include
)

# Link against common runtime library and IPC library
target_link_libraries(faultmgr PRIVATE
    # TODO: Add runtime and ipc libraries when available
)

bharat_configure_userspace_binary(faultmgr)
