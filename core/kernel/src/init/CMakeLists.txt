set(BHARAT_BOOT_PROTOCOLS_ROOT_CANDIDATES
    ${CMAKE_SOURCE_DIR}/core/boot/protocols
    ${CMAKE_SOURCE_DIR}/boot/protocols
)
set(BHARAT_BOOT_PROTOCOLS_ROOT "")
foreach(_boot_protocols_root IN LISTS BHARAT_BOOT_PROTOCOLS_ROOT_CANDIDATES)
    if(EXISTS "${_boot_protocols_root}/boot_args.c")
        set(BHARAT_BOOT_PROTOCOLS_ROOT "${_boot_protocols_root}")
        if(_boot_protocols_root MATCHES "/boot/protocols$" AND NOT _boot_protocols_root MATCHES "/core/boot/protocols$")
            message(WARNING "[migration-warning] Using legacy boot protocols root: ${_boot_protocols_root}")
        endif()
        break()
    endif()
endforeach()
if(NOT BHARAT_BOOT_PROTOCOLS_ROOT)
    message(FATAL_ERROR "No boot protocols root found in core/boot/protocols or boot/protocols")
endif()

add_library(k_init OBJECT
    ${CMAKE_SOURCE_DIR}/kernel/src/main.c
    init_bootstrap.c
    ${BHARAT_BOOT_PROTOCOLS_ROOT}/boot_args.c
    ${CMAKE_SOURCE_DIR}/kernel/src/device/device_manager.c
    ${CMAKE_SOURCE_DIR}/kernel/src/device/device_dma.c
    ${CMAKE_SOURCE_DIR}/kernel/src/device/builtin_drivers.c
    ${CMAKE_SOURCE_DIR}/kernel/src/device/pci.c
    ${CMAKE_SOURCE_DIR}/kernel/src/device/irq_domain.c
    ${CMAKE_SOURCE_DIR}/kernel/src/power_thermal_perf.c
)

target_include_directories(k_init PRIVATE
    ${CMAKE_SOURCE_DIR}/kernel/include
    ${CMAKE_SOURCE_DIR}/kernel/include/generated
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/lib/include
    ${BHARAT_BOOT_INCLUDE_DIR}
    ${CMAKE_SOURCE_DIR}/services/core/subsysmgr/include
)
target_link_libraries(k_init PRIVATE bharat_freestanding bharat_kernel_buildopts)
