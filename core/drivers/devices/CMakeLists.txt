set(BHARAT_DRIVER_DEVICES_SOURCES)

if(BHARAT_ENABLE_DRIVER_PTP_CLOCK)
    list(APPEND BHARAT_DRIVER_DEVICES_SOURCES ptp_clock/ptp_clock.c)
endif()

if(BHARAT_ENABLE_DRIVER_FPGA_MGR)
    list(APPEND BHARAT_DRIVER_DEVICES_SOURCES fpga_mgr/fpga_mgr.c)
endif()

if(BHARAT_ENABLE_DRIVER_CAN_LOOPBACK)
    list(APPEND BHARAT_DRIVER_DEVICES_SOURCES can_loopback/can_loopback.c)
endif()

if(BHARAT_ENABLE_DRIVER_VIRT_CAN)
    list(APPEND BHARAT_DRIVER_DEVICES_SOURCES virt_can/virt_can.c)
endif()

if(NOT BHARAT_DRIVER_DEVICES_SOURCES)
    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/dummy_devices.c "")
    list(APPEND BHARAT_DRIVER_DEVICES_SOURCES ${CMAKE_CURRENT_BINARY_DIR}/dummy_devices.c)
endif()

add_library(bharatos_driver_devices STATIC ${BHARAT_DRIVER_DEVICES_SOURCES})
target_include_directories(bharatos_driver_devices PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include)
target_include_directories(bharatos_driver_devices PRIVATE
    ${CMAKE_SOURCE_DIR}/core/lib/packet/include
    ${CMAKE_SOURCE_DIR}/interface/include
    ${CMAKE_SOURCE_DIR}/core/kernel/include
)
target_link_libraries(bharatos_driver_devices PRIVATE bharat_kernel_buildopts bharat_freestanding bharat_libpacket bharat_generated_includes)
target_compile_options(bharatos_driver_devices PRIVATE -ffreestanding -nostdlib -Wall)
