#ifndef BHARAT_FAULT_DIAG_H
#define BHARAT_FAULT_DIAG_H

#include "panic.h"
#include <stdint.h>
#include <stddef.h>

#define FAULT_DIAG_MAX_CORES 8 // Must match MAX_SUPPORTED_CORES or at least handle typical configs

// Core-local diagnostic breadcrumbs
typedef struct {
    uint64_t last_syscall_nr;
    uint64_t last_fault_va;
    uint64_t last_trap_cause;
    uint64_t last_exception_vector;
} fault_breadcrumbs_t;

// Record breadcrumbs
void fault_diag_record_syscall(uint64_t syscall_nr);
void fault_diag_record_fault(uint64_t fault_va, uint64_t trap_cause);

// Retrieve breadcrumbs for current core
const fault_breadcrumbs_t* fault_diag_get_breadcrumbs(void);

// Formatting helpers (reusable)
void panic_emit_header(const panic_context_t *ctx);
void panic_emit_thread_info(const panic_context_t *ctx);
void panic_emit_fault_info(const panic_context_t *ctx);
void panic_emit_breadcrumbs(const panic_context_t *ctx);

#endif // BHARAT_FAULT_DIAG_H
