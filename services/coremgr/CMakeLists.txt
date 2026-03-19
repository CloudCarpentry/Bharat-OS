cmake_minimum_required(VERSION 3.20)

project(coremgr C ASM)

# Add source files
set(SOURCES main.c)

# Create executable
add_executable(coremgr ${SOURCES})

# Target include directories
target_include_directories(coremgr PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# Link against common runtime library and IPC library (assuming lib/runtime, lib/ipc)
target_link_libraries(coremgr PRIVATE
    # TODO: Add runtime and ipc libraries when available
)

# Optional: Set properties like output directory if needed
