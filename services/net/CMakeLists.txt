cmake_minimum_required(VERSION 3.20)
project(BharatOS_Net_Service C ASM)

add_executable(net_service main.c)

target_include_directories(net_service PRIVATE
    ../../kernel/include
    ../../subsys/include
    ../../lib/urpc
)

target_compile_options(net_service PRIVATE -ffreestanding -nostdlib -Wall)

# Link against subsystem and URPC
target_link_libraries(net_service PRIVATE urpc subsys_manager)
