cmake_minimum_required(VERSION 3.20)
project(libpacket C)

add_library(packet STATIC src/packet.c)
target_include_directories(packet PUBLIC include)
target_compile_options(packet PRIVATE -Wall -ffreestanding -nostdlib)
