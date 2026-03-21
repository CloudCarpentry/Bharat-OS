cmake_minimum_required(VERSION 3.20)
project(BharatOS_Drivers C ASM)

set(BHARAT_DRIVER_SOURCES)

if(BHARAT_ENABLE_DRIVER_NET)
    list(APPEND BHARAT_DRIVER_SOURCES
        net/ptp_clock.c
        net/virtio_net.c
    )
endif()

if(BHARAT_ENABLE_DRIVER_ACCEL)
    list(APPEND BHARAT_DRIVER_SOURCES
        accel/fpga_mgr.c
    )
endif()

if(BHARAT_ENABLE_DRIVER_CLUSTER_BUS)
    list(APPEND BHARAT_DRIVER_SOURCES
        bus/cluster_bus.c
    )
endif()

if(BHARAT_ENABLE_DRIVER_AUTOMOTIVE_CAN)
    list(APPEND BHARAT_DRIVER_SOURCES
        src/automotive/can_loopback.c
    )
endif()

if(NOT BHARAT_DRIVER_SOURCES)
    message(FATAL_ERROR "No drivers enabled. Check component policy and build options.")
endif()

add_library(bharatos_drivers STATIC ${BHARAT_DRIVER_SOURCES})

target_include_directories(bharatos_drivers PUBLIC include)

target_include_directories(bharatos_drivers PRIVATE ${CMAKE_SOURCE_DIR}/lib/packet/include)
target_link_libraries(bharatos_drivers PRIVATE bharat_kernel_buildopts bharat_freestanding bharat_libpacket)

target_compile_options(bharatos_drivers PRIVATE -ffreestanding -nostdlib -Wall)
