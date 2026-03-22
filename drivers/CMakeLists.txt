cmake_minimum_required(VERSION 3.20)
project(BharatOS_Drivers C ASM)

if(COMMAND bharat_component_option)
    bharat_component_option(BHARAT_ENABLE_DRIVER_PTP_CLOCK ON "Build PTP clock driver")
    bharat_component_option(BHARAT_ENABLE_DRIVER_VIRTIO_NET ON "Build virtio net driver")
    bharat_component_option(BHARAT_ENABLE_DRIVER_FPGA_MGR ON "Build FPGA manager driver")
    bharat_component_option(BHARAT_ENABLE_DRIVER_CAN_LOOPBACK OFF "Build CAN loopback driver")
    bharat_component_option(BHARAT_ENABLE_DRIVER_CAN_CORE ON "Build CAN generic core driver")
    bharat_component_option(BHARAT_ENABLE_DRIVER_VIRT_CAN ON "Build virt_can virtual controller")
    bharat_component_option(BHARAT_ENABLE_DRIVER_GENERIC_REGISTRY ON "Build generic driver registry")
    bharat_component_option(BHARAT_ENABLE_DRIVER_BOARD_FABRIC ON "Build board fabric generic driver")
else()
    option(BHARAT_ENABLE_DRIVER_PTP_CLOCK "Build PTP clock driver" ON)
    option(BHARAT_ENABLE_DRIVER_VIRTIO_NET "Build virtio net driver" ON)
    option(BHARAT_ENABLE_DRIVER_FPGA_MGR "Build FPGA manager driver" ON)
    option(BHARAT_ENABLE_DRIVER_CAN_LOOPBACK "Build CAN loopback driver" OFF)
    option(BHARAT_ENABLE_DRIVER_CAN_CORE "Build CAN generic core driver" ON)
    option(BHARAT_ENABLE_DRIVER_VIRT_CAN "Build virt_can virtual controller" ON)
    option(BHARAT_ENABLE_DRIVER_GENERIC_REGISTRY "Build generic driver registry" ON)
    option(BHARAT_ENABLE_DRIVER_BOARD_FABRIC "Build board fabric generic driver" ON)
endif()

add_subdirectory(bus)
add_subdirectory(class)
add_subdirectory(devices)
add_subdirectory(registry)

add_library(bharatos_drivers INTERFACE)
target_link_libraries(bharatos_drivers INTERFACE
  bharatos_driver_bus
  bharatos_driver_class
  bharatos_driver_devices
  bharatos_driver_registry
)
target_include_directories(bharatos_drivers INTERFACE include)
