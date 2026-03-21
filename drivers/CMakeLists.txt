cmake_minimum_required(VERSION 3.20)
project(BharatOS_Drivers C ASM)

set(BHARAT_DRIVER_SOURCES)

list(APPEND BHARAT_DRIVER_SOURCES
    driver_version_registry/driver_version_registry.c
)

if(COMMAND bharat_component_option)
    bharat_component_option(BHARAT_ENABLE_DRIVER_PTP_CLOCK ON "Build PTP clock driver")
    bharat_component_option(BHARAT_ENABLE_DRIVER_VIRTIO_NET ON "Build virtio net driver")
    bharat_component_option(BHARAT_ENABLE_DRIVER_FPGA_MGR ON "Build FPGA manager driver")
    bharat_component_option(BHARAT_ENABLE_DRIVER_CAN_LOOPBACK OFF "Build CAN loopback driver")
    bharat_component_option(BHARAT_ENABLE_DRIVER_CAN_CORE ON "Build CAN generic core driver")
    bharat_component_option(BHARAT_ENABLE_DRIVER_VIRT_CAN ON "Build virt_can virtual controller")
    bharat_component_option(BHARAT_ENABLE_DRIVER_GENERIC_REGISTRY ON "Build generic driver registry")
    bharat_component_option(BHARAT_ENABLE_DRIVER_ARCH_IRQ_TIMER ON "Build architecture IRQ/timer generic driver")
    bharat_component_option(BHARAT_ENABLE_DRIVER_BOARD_FABRIC ON "Build board fabric generic driver")
else()
    option(BHARAT_ENABLE_DRIVER_PTP_CLOCK "Build PTP clock driver" ON)
    option(BHARAT_ENABLE_DRIVER_VIRTIO_NET "Build virtio net driver" ON)
    option(BHARAT_ENABLE_DRIVER_FPGA_MGR "Build FPGA manager driver" ON)
    option(BHARAT_ENABLE_DRIVER_CAN_LOOPBACK "Build CAN loopback driver" OFF)
    option(BHARAT_ENABLE_DRIVER_CAN_CORE "Build CAN generic core driver" ON)
    option(BHARAT_ENABLE_DRIVER_VIRT_CAN "Build virt_can virtual controller" ON)
    option(BHARAT_ENABLE_DRIVER_GENERIC_REGISTRY "Build generic driver registry" ON)
    option(BHARAT_ENABLE_DRIVER_ARCH_IRQ_TIMER "Build architecture IRQ/timer generic driver" ON)
    option(BHARAT_ENABLE_DRIVER_BOARD_FABRIC "Build board fabric generic driver" ON)
endif()

if(BHARAT_ENABLE_DRIVER_PTP_CLOCK)
    list(APPEND BHARAT_DRIVER_SOURCES
        ptp_clock/ptp_clock.c
    )
endif()

if(BHARAT_ENABLE_DRIVER_VIRTIO_NET)
    list(APPEND BHARAT_DRIVER_SOURCES
        virtio_net/virtio_net.c
    )
endif()

if(BHARAT_ENABLE_DRIVER_FPGA_MGR)
    list(APPEND BHARAT_DRIVER_SOURCES
        fpga_mgr/fpga_mgr.c
    )
endif()

if(BHARAT_ENABLE_DRIVER_CLUSTER_BUS)
    list(APPEND BHARAT_DRIVER_SOURCES
        cluster_bus/cluster_bus.c
    )
endif()

if(BHARAT_ENABLE_DRIVER_CAN_LOOPBACK)
    list(APPEND BHARAT_DRIVER_SOURCES
        can_loopback/can_loopback.c
    )
endif()

if(BHARAT_ENABLE_DRIVER_CAN_CORE)
    list(APPEND BHARAT_DRIVER_SOURCES
        can_core/can_controller_core.c
        can_core/can_filter_compile.c
    )
endif()

if(BHARAT_ENABLE_DRIVER_VIRT_CAN)
    list(APPEND BHARAT_DRIVER_SOURCES
        virt_can/virt_can.c
    )
endif()

if(BHARAT_ENABLE_DRIVER_GENERIC_REGISTRY
   OR BHARAT_ENABLE_DRIVER_ARCH_IRQ_TIMER
   OR BHARAT_ENABLE_DRIVER_BOARD_FABRIC)
    string(TOUPPER "${BHARAT_ARCH_FAMILY}" _bharat_arch_family_upper)
    string(TOLOWER "${BHARAT_TARGET_BOARD}" _bharat_target_board_lower)
    set(_bharat_generic_driver_defs)

    if(BHARAT_ENABLE_DRIVER_GENERIC_REGISTRY)
        list(APPEND BHARAT_DRIVER_SOURCES
            generic_registry/generic_registry.c
        )
    endif()

    if(BHARAT_ENABLE_DRIVER_ARCH_IRQ_TIMER AND (_bharat_arch_family_upper STREQUAL "X86"
       OR _bharat_arch_family_upper STREQUAL "ARM"
       OR _bharat_arch_family_upper STREQUAL "RISCV"))
        list(APPEND BHARAT_DRIVER_SOURCES
            arch_irq_timer/arch_irq_timer.c
        )
        list(APPEND _bharat_generic_driver_defs BHARAT_GENERIC_ENABLE_ARCH_IRQ_TIMER)
    endif()

    if(BHARAT_ENABLE_DRIVER_BOARD_FABRIC AND (_bharat_target_board_lower MATCHES "qemu"
       OR _bharat_target_board_lower MATCHES "virt"
       OR _bharat_target_board_lower MATCHES "shakti"))
        list(APPEND BHARAT_DRIVER_SOURCES
            board_fabric/board_fabric.c
        )
        list(APPEND _bharat_generic_driver_defs BHARAT_GENERIC_ENABLE_BOARD_FABRIC)
    endif()
endif()

if(NOT BHARAT_DRIVER_SOURCES)
    message(FATAL_ERROR "No drivers enabled. Check component policy and build options.")
endif()

add_library(bharatos_drivers STATIC ${BHARAT_DRIVER_SOURCES})

target_include_directories(bharatos_drivers PUBLIC include)

target_include_directories(bharatos_drivers PRIVATE ${CMAKE_SOURCE_DIR}/lib/packet/include ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(bharatos_drivers PRIVATE bharat_kernel_buildopts bharat_freestanding bharat_libpacket)

target_compile_options(bharatos_drivers PRIVATE -ffreestanding -nostdlib -Wall)

if(_bharat_generic_driver_defs)
    target_compile_definitions(bharatos_drivers PRIVATE ${_bharat_generic_driver_defs})
endif()

if (BHARAT_AUTOMOTIVE_PROFILE)
    # Generic Sensor Driver Core Library
    add_library(bharat_driver_sensor STATIC
        sensor_core/sensor_core.c
    )
    target_include_directories(bharat_driver_sensor PUBLIC include)
    target_link_libraries(bharat_driver_sensor PRIVATE bharat_libruntime)
endif()

if (BHARAT_AUTOMOTIVE_PROFILE)
    # Generic Actuator Driver Core Library
    add_library(bharat_driver_actuator STATIC
        actuator_core/actuator_core.c
    )
    target_include_directories(bharat_driver_actuator PUBLIC include)
    target_link_libraries(bharat_driver_actuator PRIVATE bharat_libruntime)
endif()

if (BHARAT_AUTOMOTIVE_PROFILE)
    # Generic Motor Driver Core Library
    add_library(bharat_driver_motor STATIC
        motor_core/motor_control_core.c
        motor_core/pwm_motor_backend.c
        motor_core/qei_backend.c
    )
    target_include_directories(bharat_driver_motor PUBLIC include)
    target_link_libraries(bharat_driver_motor PRIVATE bharat_libruntime)
endif()
