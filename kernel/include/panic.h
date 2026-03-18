#ifndef BHARAT_PANIC_H
#define BHARAT_PANIC_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef struct panic_context {
    const char *message;
    const char *cause_str;
    uint64_t cause_code;
    uint64_t fault_addr;
    uint64_t ip;
    uint64_t sp;
    uint64_t core_id;
    uint64_t thread_id;
    uint64_t process_id;
    uint64_t aspace_id;
    uint64_t last_syscall_nr;
    const void *trap_frame;
    uint32_t flags;
} panic_context_t;

// Standard kernel panic, wrappers internally over kernel_panic_ex
void kernel_panic(const char *message);

// Extended kernel panic with structured diagnostic context
void kernel_panic_ex(const panic_context_t *ctx);

// Hooks for flushing logs safely during a panic
void panic_flush_logs(void);

#endif // BHARAT_PANIC_H
