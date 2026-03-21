#ifndef BHARAT_KERNEL_CONSOLE_H
#define BHARAT_KERNEL_CONSOLE_H

#include "console_types.h"
#include "console_caps.h"
#include "console_backend.h"

/*
 * Console Subsystem Initialization (Phases)
 */
void console_init_early(void);
void console_init_runtime(void);
void console_panic_mode(void);

/*
 * Backend Registration and Routing
 */
int console_register_backend(console_backend_t* backend);
void console_set_routing_policy(console_backend_type_t preferred_type, console_log_level_t min_level);

/*
 * Logging Functions
 */
void console_log(console_log_level_t level, const char* fmt, ...);
void console_write_raw(const char* str);
void console_write_atomic(const char* str); // Use during panics/exceptions

/*
 * Helper / Buffer Management
 */
void console_flush_all(void);
console_phase_t console_get_phase(void);

#endif /* BHARAT_KERNEL_CONSOLE_H */
