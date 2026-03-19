cmake_minimum_required(VERSION 3.20)
project(netfast C)

add_executable(netfast src/main.c)
target_compile_options(netfast PRIVATE -Wall -ffreestanding -nostdlib )
target_link_options(netfast PRIVATE -nostdlib -no-pie)
# Link against necessary system primitives/stubs here in the future
