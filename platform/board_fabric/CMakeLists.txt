if(BHARAT_ARCH_FAMILY STREQUAL "X86" AND BHARAT_ARCH_BITS STREQUAL "64")
    set(PLATFORM_UART_SRC "../boards/qemu-virt-x86_64/platform_uart.c")
elseif(BHARAT_ARCH_FAMILY STREQUAL "ARM" AND BHARAT_ARCH_BITS STREQUAL "64")
    set(PLATFORM_UART_SRC "../boards/qemu-virt-arm64/platform_uart.c")
elseif(BHARAT_ARCH_FAMILY STREQUAL "ARM" AND BHARAT_ARCH_BITS STREQUAL "32")
    set(PLATFORM_UART_SRC "../boards/qemu-virt-arm32/platform_uart.c")
elseif(BHARAT_ARCH_FAMILY STREQUAL "RISCV" AND BHARAT_ARCH_BITS STREQUAL "64")
    set(PLATFORM_UART_SRC "../boards/qemu-virt-riscv64/platform_uart.c")
elseif(BHARAT_ARCH_FAMILY STREQUAL "RISCV" AND BHARAT_ARCH_BITS STREQUAL "32")
    set(PLATFORM_UART_SRC "../boards/qemu-virt-riscv32/platform_uart.c")
endif()

add_library(board_fabric STATIC
    board_fabric.c
    boot_console.c
    ${PLATFORM_UART_SRC}
)
target_include_directories(board_fabric PRIVATE ${CMAKE_SOURCE_DIR}/kernel/include ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/drivers/include)
target_link_libraries(board_fabric PRIVATE bharat_kernel_buildopts bharat_freestanding bharat_platform_headers)
