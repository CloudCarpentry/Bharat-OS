cmake_minimum_required(VERSION 3.20)

project(faultmgr C ASM)

# Add source files
set(SOURCES main.c)

# Create executable
add_executable(faultmgr ${SOURCES})

# Target include directories
target_include_directories(faultmgr PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# Link against common runtime library and IPC library
target_link_libraries(faultmgr PRIVATE
    # TODO: Add runtime and ipc libraries when available
)
