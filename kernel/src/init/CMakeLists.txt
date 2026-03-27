add_library(k_init OBJECT
    ../../src/main.c
    init_bootstrap.c
    ../../../boot/protocols/boot_args.c
    ../../src/device/device_manager.c
    ../../src/device/device_dma.c
    ../../src/device/builtin_drivers.c
    ../../src/device/pci.c
    ../../src/device/irq_domain.c
    ../../src/power_thermal_perf.c
)

target_include_directories(k_init PRIVATE ../../include ../../include/generated ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/lib/include ${CMAKE_SOURCE_DIR}/boot/include ${CMAKE_SOURCE_DIR}/services/core/subsysmgr/include)
target_link_libraries(k_init PRIVATE bharat_freestanding bharat_kernel_buildopts)
