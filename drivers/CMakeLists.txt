cmake_minimum_required(VERSION 3.20)
project(BharatOS_Drivers C ASM)

set(BHARAT_DRIVER_SOURCES)

list(APPEND BHARAT_DRIVER_SOURCES
    version/driver_version_registry.c
)

if(COMMAND bharat_component_option)
    bharat_component_option(BHARAT_ENABLE_DRIVER_GENERIC ON "Build architecture/board generic drivers")
else()
    option(BHARAT_ENABLE_DRIVER_GENERIC "Build architecture/board generic drivers" ON)
endif()

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

if(BHARAT_ENABLE_DRIVER_GENERIC)
    string(TOUPPER "${BHARAT_ARCH_FAMILY}" _bharat_arch_family_upper)
    string(TOLOWER "${BHARAT_TARGET_BOARD}" _bharat_target_board_lower)
    set(_bharat_generic_driver_defs)

    list(APPEND BHARAT_DRIVER_SOURCES
        generic/generic_registry.c
    )

    if(_bharat_arch_family_upper STREQUAL "X86"
       OR _bharat_arch_family_upper STREQUAL "ARM"
       OR _bharat_arch_family_upper STREQUAL "RISCV")
        list(APPEND BHARAT_DRIVER_SOURCES
            generic/arch_irq_timer.c
        )
        list(APPEND _bharat_generic_driver_defs BHARAT_GENERIC_ENABLE_ARCH_IRQ_TIMER)
    endif()

    if(_bharat_target_board_lower MATCHES "qemu"
       OR _bharat_target_board_lower MATCHES "virt"
       OR _bharat_target_board_lower MATCHES "shakti")
        list(APPEND BHARAT_DRIVER_SOURCES
            generic/board_fabric.c
        )
        list(APPEND _bharat_generic_driver_defs BHARAT_GENERIC_ENABLE_BOARD_FABRIC)
    endif()
endif()

if(NOT BHARAT_DRIVER_SOURCES)
    message(FATAL_ERROR "No drivers enabled. Check component policy and build options.")
endif()

add_library(bharatos_drivers STATIC ${BHARAT_DRIVER_SOURCES})

target_include_directories(bharatos_drivers PUBLIC include)

target_include_directories(bharatos_drivers PRIVATE ${CMAKE_SOURCE_DIR}/lib/packet/include)
target_link_libraries(bharatos_drivers PRIVATE bharat_kernel_buildopts bharat_freestanding bharat_libpacket)

target_compile_options(bharatos_drivers PRIVATE -ffreestanding -nostdlib -Wall)

if(_bharat_generic_driver_defs)
    target_compile_definitions(bharatos_drivers PRIVATE ${_bharat_generic_driver_defs})
endif()
