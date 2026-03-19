cmake_minimum_required(VERSION 3.20)
project(netstack C)

add_executable(netstack src/main.c)
target_compile_options(netstack PRIVATE -Wall -ffreestanding -nostdlib -fPIE -fPIC)
target_link_options(netstack PRIVATE -nostdlib -no-pie)
# Link against necessary system primitives/stubs here in the future
