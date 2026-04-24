set(BHARAT_BOOT_PROTOCOLS_ROOT "${CMAKE_SOURCE_DIR}/core/boot/protocols")
if(NOT EXISTS "${BHARAT_BOOT_PROTOCOLS_ROOT}/boot_args.c")
    message(FATAL_ERROR "Missing canonical boot protocols root: ${BHARAT_BOOT_PROTOCOLS_ROOT}")
endif()

add_library(k_init OBJECT
    ${BHARAT_KERNEL_SRC_ROOT}/main.c
    init_bootstrap.c
    ${BHARAT_BOOT_PROTOCOLS_ROOT}/boot_args.c
    ${BHARAT_KERNEL_SRC_ROOT}/device/device_manager.c
    ${BHARAT_KERNEL_SRC_ROOT}/device/device_dma.c
    ${BHARAT_KERNEL_SRC_ROOT}/device/builtin_drivers.c
    ${BHARAT_KERNEL_SRC_ROOT}/device/pci.c
    ${BHARAT_KERNEL_SRC_ROOT}/device/irq_domain.c
    ${BHARAT_KERNEL_SRC_ROOT}/power_thermal_perf.c
)

target_include_directories(k_init PRIVATE
    ${BHARAT_KERNEL_INCLUDE_DIR}
    ${BHARAT_KERNEL_INCLUDE_DIR}/generated
    ${CMAKE_SOURCE_DIR}/interface/include
    ${BHARAT_LIB_ROOT}/include
    ${BHARAT_BOOT_INCLUDE_DIR}
    ${CMAKE_SOURCE_DIR}/services/core/subsysmgr/include
)
target_link_libraries(k_init PRIVATE bharat_freestanding bharat_kernel_buildopts)
