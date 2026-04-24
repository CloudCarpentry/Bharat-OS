cmake_minimum_required(VERSION 3.20)

project(telemetrymgr C ASM)

# Add source files
set(SOURCES
    main.c
    ../common/thermal_policy.c
)

# Create executable
add_executable(telemetrymgr ${SOURCES})
    target_link_libraries(telemetrymgr PRIVATE bharat_user_buildopts bharat_freestanding)

# Target include directories
target_include_directories(telemetrymgr PRIVATE
    ${CMAKE_SOURCE_DIR}/interface/include
    ${CMAKE_SOURCE_DIR}/services/include
)

# Link against common runtime library and IPC library
target_link_libraries(telemetrymgr PRIVATE
    # TODO: Add runtime and ipc libraries when available
)

bharat_configure_userspace_binary(telemetrymgr)
