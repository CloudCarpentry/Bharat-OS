cmake_minimum_required(VERSION 3.20)
project(boot_displayd C)

add_executable(boot_displayd main.c)

target_include_directories(boot_displayd PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/kernel/include
)

# Use PIC to fix linker issues since the environment defaults to PIE
target_compile_options(boot_displayd PRIVATE -fPIC)
target_link_options(boot_displayd PRIVATE "-no-pie")

# In real scenarios we would link against the capability and IPC libraries
target_link_libraries(boot_displayd PRIVATE subsys_manager)
