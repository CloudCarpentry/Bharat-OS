cmake_minimum_required(VERSION 3.20)
project(libnet C)

add_library(bharat_libnet STATIC src/net.c)
target_include_directories(bharat_libnet PUBLIC include)
target_link_libraries(bharat_libnet PUBLIC bharat_freestanding bharat_kernel_buildopts)

# Alias for compatibility
add_library(net ALIAS bharat_libnet)
