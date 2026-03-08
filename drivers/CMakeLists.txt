cmake_minimum_required(VERSION 3.20)
project(BharatOS_Drivers C ASM)

# Add driver subdirectories
add_library(bharatos_drivers STATIC
    net/ptp_clock.c
    accel/fpga_mgr.c
    bus/cluster_bus.c
    src/automotive/can_loopback.c
)

target_include_directories(bharatos_drivers PUBLIC include)

target_compile_options(bharatos_drivers PRIVATE -ffreestanding -nostdlib -Wall)
