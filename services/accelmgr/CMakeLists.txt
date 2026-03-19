cmake_minimum_required(VERSION 3.20)

project(accelmgr C ASM)

# Add source files
set(SOURCES main.c)

# Create executable
add_executable(accelmgr ${SOURCES})

# Target include directories
target_include_directories(accelmgr PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# Link against common runtime library and IPC library
target_link_libraries(accelmgr PRIVATE
    # TODO: Add runtime and ipc libraries when available
)
