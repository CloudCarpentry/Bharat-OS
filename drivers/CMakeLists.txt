cmake_minimum_required(VERSION 3.20)
project(BharatOS_Drivers C ASM)

# Add driver subdirectories
add_library(bharatos_drivers STATIC
    net/ptp_clock.c
    net/virtio_net.c
    accel/fpga_mgr.c
    bus/cluster_bus.c
    src/automotive/can_loopback.c
    src/console/fb_console.c
)

target_include_directories(bharatos_drivers PUBLIC include)

target_include_directories(bharatos_drivers PRIVATE ${CMAKE_SOURCE_DIR}/lib/packet/include ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(bharatos_drivers PRIVATE bharat_kernel_buildopts bharat_freestanding bharat_libpacket)

target_compile_options(bharatos_drivers PRIVATE -ffreestanding -nostdlib -Wall)
