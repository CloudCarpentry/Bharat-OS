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

    # Driver Core Baseline & Edge I/O
    bharat_component_option(BHARAT_ENABLE_DRIVER_CORE ON "Build driver core baseline")
    bharat_component_option(BHARAT_ENABLE_GPIO ON "Build GPIO core driver")
    bharat_component_option(BHARAT_ENABLE_PINCTRL ON "Build pinctrl core driver")
    bharat_component_option(BHARAT_ENABLE_PWM ON "Build PWM core driver")
    bharat_component_option(BHARAT_ENABLE_LED ON "Build LED subsystem driver")
    bharat_component_option(BHARAT_ENABLE_INPUT_GPIO_KEYS ON "Build gpio-keys input driver")
    bharat_component_option(BHARAT_ENABLE_USB_CORE_HEADERS ON "Enable USB core contracts")

    # Storage and Bus Drivers
    bharat_component_option(BHARAT_ENABLE_DRIVER_NVME OFF "Build NVMe driver")
    bharat_component_option(BHARAT_ENABLE_DRIVER_SDHOST OFF "Build SD/eMMC host driver")
    bharat_component_option(BHARAT_ENABLE_DRIVER_I2C_CORE OFF "Build I2C core driver")
    bharat_component_option(BHARAT_ENABLE_DRIVER_SPI_CORE OFF "Build SPI core driver")
    bharat_component_option(BHARAT_ENABLE_DRIVER_PCIE_HOST OFF "Build PCIe host bridge baseline driver")

    # Input and Sensor Drivers
    bharat_component_option(BHARAT_ENABLE_DRIVER_VIRTIO_INPUT OFF "Build virtio input driver")
    bharat_component_option(BHARAT_ENABLE_DRIVER_I2C_HID OFF "Build I2C HID driver")
    bharat_component_option(BHARAT_ENABLE_DRIVER_IMU OFF "Build Generic IMU driver")

    # Display and GPU Drivers
    bharat_component_option(BHARAT_ENABLE_DRIVER_DRM_CORE OFF "Build DRM core driver")
    bharat_component_option(BHARAT_ENABLE_DRIVER_VIRTIO_GPU OFF "Build virtio GPU driver")
else()
    option(BHARAT_ENABLE_DRIVER_PTP_CLOCK "Build PTP clock driver" ON)
    option(BHARAT_ENABLE_DRIVER_VIRTIO_NET "Build virtio net driver" ON)
    option(BHARAT_ENABLE_DRIVER_FPGA_MGR "Build FPGA manager driver" ON)
    option(BHARAT_ENABLE_DRIVER_CAN_LOOPBACK "Build CAN loopback driver" OFF)
    option(BHARAT_ENABLE_DRIVER_CAN_CORE "Build CAN generic core driver" ON)
    option(BHARAT_ENABLE_DRIVER_VIRT_CAN "Build virt_can virtual controller" ON)
    option(BHARAT_ENABLE_DRIVER_GENERIC_REGISTRY "Build generic driver registry" ON)
    option(BHARAT_ENABLE_DRIVER_BOARD_FABRIC "Build board fabric generic driver" ON)

    # Driver Core Baseline & Edge I/O
    option(BHARAT_ENABLE_DRIVER_CORE "Build driver core baseline" ON)
    option(BHARAT_ENABLE_GPIO "Build GPIO core driver" ON)
    option(BHARAT_ENABLE_PINCTRL "Build pinctrl core driver" ON)
    option(BHARAT_ENABLE_PWM "Build PWM core driver" ON)
    option(BHARAT_ENABLE_LED "Build LED subsystem driver" ON)
    option(BHARAT_ENABLE_INPUT_GPIO_KEYS "Build gpio-keys input driver" ON)
    option(BHARAT_ENABLE_USB_CORE_HEADERS "Enable USB core contracts" ON)

    # Storage and Bus Drivers
    option(BHARAT_ENABLE_DRIVER_NVME "Build NVMe driver" OFF)
    option(BHARAT_ENABLE_DRIVER_SDHOST "Build SD/eMMC host driver" OFF)
    option(BHARAT_ENABLE_DRIVER_I2C_CORE "Build I2C core driver" OFF)
    option(BHARAT_ENABLE_DRIVER_SPI_CORE "Build SPI core driver" OFF)
    option(BHARAT_ENABLE_DRIVER_PCIE_HOST "Build PCIe host bridge baseline driver" OFF)

    # Input and Sensor Drivers
    option(BHARAT_ENABLE_DRIVER_VIRTIO_INPUT "Build virtio input driver" OFF)
    option(BHARAT_ENABLE_DRIVER_I2C_HID "Build I2C HID driver" OFF)
    option(BHARAT_ENABLE_DRIVER_IMU "Build Generic IMU driver" OFF)

    # Display and GPU Drivers
    option(BHARAT_ENABLE_DRIVER_DRM_CORE "Build DRM core driver" OFF)
    option(BHARAT_ENABLE_DRIVER_VIRTIO_GPU "Build virtio GPU driver" OFF)
endif()

add_subdirectory(bus)
add_subdirectory(class)
add_subdirectory(devices)
add_subdirectory(serial)
add_subdirectory(registry)
add_subdirectory(accel)
add_subdirectory(net)
add_subdirectory(display)
add_subdirectory(storage)
add_subdirectory(input)
add_subdirectory(block/virtio_blk)

# Driver Core Baseline Subdirectories
if(BHARAT_ENABLE_DRIVER_CORE)
    add_subdirectory(core)
endif()

if(BHARAT_ENABLE_LED)
    add_subdirectory(led)
endif()

add_library(bharatos_drivers INTERFACE)
target_link_libraries(bharatos_drivers INTERFACE
  bharatos_driver_bus
  bharatos_driver_class
  bharatos_driver_devices
  bharatos_driver_registry
  bharatos_driver_net
  bharatos_driver_display
  bharatos_driver_storage
  bharatos_driver_input
  driver_virtio_blk
)

if(BHARAT_ENABLE_DRIVER_CORE)
  target_link_libraries(bharatos_drivers INTERFACE bharatos_driver_baseline_core)
endif()

if(BHARAT_ENABLE_LED)
  target_link_libraries(bharatos_drivers INTERFACE bharatos_driver_baseline_led)
endif()

target_include_directories(bharatos_drivers INTERFACE include)
