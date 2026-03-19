cmake_minimum_required(VERSION 3.20)
project(BharatOS_LibNet C)

add_library(net STATIC src/net.c)
target_include_directories(net PUBLIC include)
target_compile_options(net PRIVATE -ffreestanding -nostdlib -Wall -Wextra)
