cmake_minimum_required(VERSION 3.20)

project(telemetrymgr C ASM)

# Add source files
set(SOURCES main.c)

# Create executable
add_executable(telemetrymgr ${SOURCES})

# Target include directories
target_include_directories(telemetrymgr PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# Link against common runtime library and IPC library
target_link_libraries(telemetrymgr PRIVATE
    # TODO: Add runtime and ipc libraries when available
)
