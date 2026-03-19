cmake_minimum_required(VERSION 3.20)
project(netmgr C)

add_executable(netmgr src/main.c)
target_compile_options(netmgr PRIVATE -Wall -ffreestanding -nostdlib )
target_link_options(netmgr PRIVATE -nostdlib -no-pie)
# Link against necessary system primitives/stubs here in the future
