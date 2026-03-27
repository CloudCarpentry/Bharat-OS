cmake_minimum_required(VERSION 3.20)
project(boot_displayd C)

add_executable(boot_displayd main.c)
    target_link_libraries(boot_displayd PRIVATE bharat_user_buildopts bharat_freestanding)

target_include_directories(boot_displayd PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/kernel/include
)

# Use PIC to fix linker issues since the environment defaults to PIE
target_compile_options(boot_displayd PRIVATE -fPIC)
target_link_options(boot_displayd PRIVATE "LINKER:-no-pie" "-fno-pie")

# In real scenarios we would link against the capability and IPC libraries
target_link_libraries(boot_displayd PRIVATE subsys_manager)

bharat_configure_userspace_binary(boot_displayd)
