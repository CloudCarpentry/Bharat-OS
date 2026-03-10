#ifndef BHARAT_KERNEL_CONSOLE_H
#define BHARAT_KERNEL_CONSOLE_H

#include <stdint.h>
#include <stddef.h>

/*
 * Bharat-OS Core Console Routing Interface
 *
 * Provides an abstraction for early boot logging and routing to hardware
 * backends before user-space subsystem daemons take over.
 */

typedef enum {
    CONSOLE_LEVEL_DEBUG = 0,
    CONSOLE_LEVEL_INFO,
    CONSOLE_LEVEL_WARN,
    CONSOLE_LEVEL_ERROR,
    CONSOLE_LEVEL_PANIC
} console_log_level_t;

typedef enum {
    CONSOLE_BACKEND_SERIAL = 1,
    CONSOLE_BACKEND_FRAMEBUFFER = 2,
    CONSOLE_BACKEND_NETWORK_LOG = 3
} console_backend_type_t;

/* Defines a registered console backend */
typedef struct console_backend {
    console_backend_type_t type;
    const char* name;
    int (*init)(void);
    void (*write)(const char* str, size_t len);
    void (*flush)(void);
    struct console_backend* next;
} console_backend_t;

/* Initialize the core console subsystem (early boot) */
void console_init(void);

/* Register a new output backend (e.g. UART, FB text mode) */
int console_register_backend(console_backend_t* backend);

/* Log a formatted message to all active backends */
void console_log(console_log_level_t level, const char* fmt, ...);

/* Write a raw string (bypasses formatting, used during panics) */
void console_write_raw(const char* str);

#endif /* BHARAT_KERNEL_CONSOLE_H */
