add_library(k_debug OBJECT
    ../../src/panic.c
    ../../src/trace/trace.c
    ../../src/console/console_core.c
    ../../src/console/console_buffer.c
    ../../src/console/console_discovery.c
    ../../src/console/console_format.c
    ../../src/console/console_panic.c
    ../../src/console/console_policy.c
    ../../src/console/console_render_fb.c
    ../../src/console/console_render_plain.c
    ../../src/console/console_router.c
    ../../src/console/utf8.c
    ../../src/console/framebuffer_console.c
    ../../src/console/serial_console.c
    ../../src/console/memlog_console.c
    ../../src/console/drivers/uart_pl011.c
    ../../src/console/drivers/uart_ns16550.c
    ../../src/console/drivers/uart_simple_mmio.c
)

target_include_directories(k_debug PRIVATE ../../include ../../include/generated ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(k_debug PRIVATE bharat_freestanding bharat_kernel_buildopts)
