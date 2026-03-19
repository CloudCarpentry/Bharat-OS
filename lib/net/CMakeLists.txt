cmake_minimum_required(VERSION 3.20)
project(libnet C)

add_library(net STATIC src/net.c)
target_include_directories(net PUBLIC include)
target_compile_options(net PRIVATE -Wall -ffreestanding -nostdlib)
