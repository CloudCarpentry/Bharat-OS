add_library(k_debug OBJECT
    ../../src/panic.c
    ../../src/trace/trace.c
    ../../src/debug/early_console.c
    ../../src/debug/hal_serial_compat.c
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
    ../../src/console/drivers/uart_simple_mmio.c
)

target_include_directories(k_debug PRIVATE ../../include ../../include/generated ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/lib/include ${CMAKE_SOURCE_DIR}/boot/include)
target_link_libraries(k_debug PRIVATE bharat_freestanding bharat_kernel_buildopts)
