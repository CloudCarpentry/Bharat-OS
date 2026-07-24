set(SERIAL_DRV_SRCS "")

if(BHARAT_ENABLE_DRIVER_SERIAL_PL011)
    list(APPEND SERIAL_DRV_SRCS pl011/pl011.c)
endif()

if(BHARAT_ENABLE_DRIVER_SERIAL_NS16550)
    list(APPEND SERIAL_DRV_SRCS ns16550/ns16550.c)
endif()

if(BHARAT_ENABLE_DRIVER_SERIAL_SIFIVE)
    list(APPEND SERIAL_DRV_SRCS sifive_uart/sifive_uart.c)
endif()

if(BHARAT_ENABLE_DRIVER_SERIAL_CADENCE)
    list(APPEND SERIAL_DRV_SRCS cadence_uart/cadence_uart.c)
endif()

if(BHARAT_ENABLE_DRIVER_SERIAL_IMX_LPUART)
    list(APPEND SERIAL_DRV_SRCS imx_lpuart/imx_lpuart.c)
endif()

if(BHARAT_ENABLE_DRIVER_SERIAL_DW_APB)
    list(APPEND SERIAL_DRV_SRCS dw_apb_uart/dw_apb_uart.c)
endif()

list(APPEND SERIAL_DRV_SRCS serial_console_match.c)

# Fallback to prevent empty target if no drivers are selected
if(NOT SERIAL_DRV_SRCS)
    # Create a dummy file if needed, or simply compile ns16550 as a strict last resort
    list(APPEND SERIAL_DRV_SRCS ns16550/ns16550.c)
endif()

add_library(serial_drivers OBJECT ${SERIAL_DRV_SRCS})

# Propagate feature flags to the compiler
if(BHARAT_ENABLE_DRIVER_SERIAL_PL011)
    target_compile_definitions(serial_drivers PRIVATE BHARAT_ENABLE_DRIVER_SERIAL_PL011=1)
endif()
if(BHARAT_ENABLE_DRIVER_SERIAL_NS16550)
    target_compile_definitions(serial_drivers PRIVATE BHARAT_ENABLE_DRIVER_SERIAL_NS16550=1)
endif()
if(BHARAT_ENABLE_DRIVER_SERIAL_SIFIVE)
    target_compile_definitions(serial_drivers PRIVATE BHARAT_ENABLE_DRIVER_SERIAL_SIFIVE=1)
endif()
if(BHARAT_ENABLE_DRIVER_SERIAL_CADENCE)
    target_compile_definitions(serial_drivers PRIVATE BHARAT_ENABLE_DRIVER_SERIAL_CADENCE=1)
endif()
if(BHARAT_ENABLE_DRIVER_SERIAL_IMX_LPUART)
    target_compile_definitions(serial_drivers PRIVATE BHARAT_ENABLE_DRIVER_SERIAL_IMX_LPUART=1)
endif()
if(BHARAT_ENABLE_DRIVER_SERIAL_DW_APB)
    target_compile_definitions(serial_drivers PRIVATE BHARAT_ENABLE_DRIVER_SERIAL_DW_APB=1)
endif()

# Make serial_console_match available to kernel
target_compile_options(serial_drivers PRIVATE
    $<$<COMPILE_LANGUAGE:C>:-ffreestanding>
    $<$<COMPILE_LANGUAGE:C>:-fno-builtin>
)

target_include_directories(serial_drivers PRIVATE
    ${CMAKE_SOURCE_DIR}/core/kernel/include
    ${CMAKE_SOURCE_DIR}/core/kernel/include/generated
    ${CMAKE_SOURCE_DIR}/core/drivers/include
    ${CMAKE_SOURCE_DIR}/core/platform/include
)

target_link_libraries(serial_drivers PRIVATE bharat_freestanding bharat_kernel_buildopts)
