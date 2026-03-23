add_library(serial_drivers OBJECT
    pl011/pl011.c
    ns16550/ns16550.c
)

target_include_directories(serial_drivers PRIVATE
    ${CMAKE_SOURCE_DIR}/kernel/include
    ${CMAKE_SOURCE_DIR}/kernel/include/generated
)

target_link_libraries(serial_drivers PRIVATE bharat_freestanding bharat_kernel_buildopts)
