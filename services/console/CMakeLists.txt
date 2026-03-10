cmake_minimum_required(VERSION 3.20)
project(BharatOS_Console_Service C ASM)

add_executable(console main.c)

target_include_directories(console PRIVATE
    ../../kernel/include
    ../../subsys/include
    ../../lib/urpc
)

target_compile_options(console PRIVATE -ffreestanding -nostdlib -Wall)

# Link against the subsystem manager and URPC libraries
target_link_libraries(console PRIVATE urpc subsys_manager)
