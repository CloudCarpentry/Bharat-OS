cmake_minimum_required(VERSION 3.20)

project(memmgr C ASM)

# Add source files
set(SOURCES main.c)

# Create executable
add_executable(memmgr ${SOURCES})
    target_link_libraries(memmgr PRIVATE bharat_user_buildopts bharat_freestanding)

# Target include directories
target_include_directories(memmgr PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# Link against common runtime library and IPC library
target_link_libraries(memmgr PRIVATE
    # TODO: Add runtime and ipc libraries when available
)
