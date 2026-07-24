#include "fault_diag.h"
#include "hal/hal.h"
#include "sched/sched.h"

// Hardcode max cores for diagnostic tracking to avoid dynamic allocation
#ifndef MAX_SUPPORTED_CORES
#define MAX_SUPPORTED_CORES 8U
#endif

static fault_breadcrumbs_t g_core_breadcrumbs[MAX_SUPPORTED_CORES] = {0};

void fault_diag_record_syscall(uint64_t syscall_nr) {
    uint32_t core = hal_cpu_get_id();
    if (core < MAX_SUPPORTED_CORES) {
        g_core_breadcrumbs[core].last_syscall_nr = syscall_nr;
    }
}

void fault_diag_record_fault(uint64_t fault_va, uint64_t trap_cause) {
    uint32_t core = hal_cpu_get_id();
    if (core < MAX_SUPPORTED_CORES) {
        g_core_breadcrumbs[core].last_fault_va = fault_va;
        g_core_breadcrumbs[core].last_trap_cause = trap_cause;
    }
}

const fault_breadcrumbs_t* fault_diag_get_breadcrumbs(void) {
    uint32_t core = hal_cpu_get_id();
    if (core < MAX_SUPPORTED_CORES) {
        return &g_core_breadcrumbs[core];
    }
    return NULL;
}

void panic_emit_header(const panic_context_t *ctx) {
    hal_serial_write("\n================ KERNEL PANIC ================\n");
    hal_serial_write("message      : ");
    hal_serial_write(ctx->message ? ctx->message : "(unknown)");
    hal_serial_write("\ncause        : ");
    hal_serial_write(ctx->cause_str ? ctx->cause_str : "N/A");
    hal_serial_write("\ncause_code   : ");
    hal_serial_write_hex(ctx->cause_code);
    hal_serial_write("\n");
}

void panic_emit_thread_info(const panic_context_t *ctx) {
    hal_serial_write("core         : ");
    hal_serial_write_hex(ctx->core_id);
    hal_serial_write("\nthread       : ");
    hal_serial_write_hex(ctx->thread_id);
    hal_serial_write("\nprocess      : ");
    hal_serial_write_hex(ctx->process_id);
    hal_serial_write("\naspace       : ");
    hal_serial_write_hex(ctx->aspace_id);
    hal_serial_write("\n");
}

void panic_emit_fault_info(const panic_context_t *ctx) {
    hal_serial_write("last_syscall : ");
    hal_serial_write_hex(ctx->last_syscall_nr);
    hal_serial_write("\nfault_va     : ");
    hal_serial_write_hex(ctx->fault_addr);
    hal_serial_write("\nip           : ");
    hal_serial_write_hex(ctx->ip);
    hal_serial_write("\nsp           : ");
    hal_serial_write_hex(ctx->sp);
    hal_serial_write("\n==============================================\n");
}

void panic_emit_breadcrumbs(const panic_context_t *ctx) {
    (void)ctx;
    const fault_breadcrumbs_t *bc = fault_diag_get_breadcrumbs();
    if (bc) {
        hal_serial_write("\n--- Breadcrumbs ---\n");
        hal_serial_write("last_syscall : ");
        hal_serial_write_hex(bc->last_syscall_nr);
        hal_serial_write("\nlast_fault_va: ");
        hal_serial_write_hex(bc->last_fault_va);
        hal_serial_write("\nlast_trap    : ");
        hal_serial_write_hex(bc->last_trap_cause);
        hal_serial_write("\n-------------------\n");
    }
}
