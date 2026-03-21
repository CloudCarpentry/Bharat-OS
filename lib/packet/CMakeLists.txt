cmake_minimum_required(VERSION 3.20)
project(BharatOS_Packet_Lib C)

add_library(bharat_libpacket STATIC
    src/packet.c
)

target_include_directories(bharat_libpacket PUBLIC
    include
)

target_link_libraries(bharat_libpacket PUBLIC bharat_freestanding bharat_kernel_buildopts)
bharat_configure_userspace_library(bharat_libpacket)

# Alias for compatibility
add_library(packet ALIAS bharat_libpacket)
