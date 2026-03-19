cmake_minimum_required(VERSION 3.20)
project(BharatOS_NetMgr_Service C)

add_executable(netmgr main.c)

target_include_directories(netmgr PRIVATE
    ../../kernel/include
    ../../subsys/include
    ../../lib/net/include
    ../../lib/packet/include
    ../../lib/urpc
)

target_compile_options(netmgr PRIVATE -ffreestanding -nostdlib -Wall -Wextra)

# Link against subsystem, URPC, and networking libs
target_link_libraries(netmgr PRIVATE urpc subsys_manager net packet)
