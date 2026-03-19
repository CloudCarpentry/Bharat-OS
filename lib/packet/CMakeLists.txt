cmake_minimum_required(VERSION 3.20)
project(BharatOS_LibPacket C)

add_library(packet STATIC src/packet.c)
target_include_directories(packet PUBLIC include)
target_compile_options(packet PRIVATE -ffreestanding -nostdlib -Wall -Wextra)
