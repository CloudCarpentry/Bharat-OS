cmake_minimum_required(VERSION 3.20)

project(servicemgr C ASM)

# Add source files
set(SOURCES main.c)

# Create executable
add_executable(servicemgr ${SOURCES})

# Target include directories
target_include_directories(servicemgr PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# Link against common runtime library and IPC library
target_link_libraries(servicemgr PRIVATE
    # TODO: Add runtime and ipc libraries when available
)
