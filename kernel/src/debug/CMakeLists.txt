add_library(k_debug OBJECT
    ../../src/panic.c
    ../../src/trace/trace.c
    ../../src/console/console_core.c
    ../../src/console/console_buffer.c
    ../../src/console/utf8.c
    ../../src/console/serial_backend.c
    ../../src/console/fb_backend.c
    ../../src/console/memlog_console.c
    ../../../kernel/console/fb_console.c
)

target_include_directories(k_debug PRIVATE ../../include ../../include/generated ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(k_debug PRIVATE bharat_freestanding bharat_kernel_buildopts)
