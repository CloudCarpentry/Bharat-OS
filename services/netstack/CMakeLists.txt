cmake_minimum_required(VERSION 3.20)
project(BharatOS_NetStack_Service C)

add_executable(netstack main.c)

target_include_directories(netstack PRIVATE
    ../../kernel/include
    ../../subsys/include
    ../../lib/net/include
    ../../lib/packet/include
    ../../lib/urpc
)

target_compile_options(netstack PRIVATE -ffreestanding -nostdlib -Wall -Wextra)

# Link against subsystem, URPC, and networking libs
target_link_libraries(netstack PRIVATE urpc subsys_manager net packet)
