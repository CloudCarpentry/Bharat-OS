#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    TRAP_CLASS_UNKNOWN = 0,
    TRAP_CLASS_INTERRUPT,
    TRAP_CLASS_SYSCALL,
    TRAP_CLASS_PAGE_FAULT,
    TRAP_CLASS_ACCESS_FAULT,
    TRAP_CLASS_ALIGNMENT,
    TRAP_CLASS_ILLEGAL_INSTR,
    TRAP_CLASS_BREAKPOINT,
    TRAP_CLASS_GENERAL_FAULT,
    TRAP_CLASS_TIMER,
    TRAP_CLASS_IPI,
} trap_class_t;

typedef enum {
    TRAP_ORIGIN_KERNEL = 0,
    TRAP_ORIGIN_USER,
} trap_origin_t;

typedef enum {
    BH_FAULT_ACCESS_UNKNOWN = 0,
    BH_FAULT_ACCESS_READ,
    BH_FAULT_ACCESS_WRITE,
    BH_FAULT_ACCESS_EXECUTE,
} bh_fault_access_t;

typedef enum {
    BH_FAULT_REASON_UNKNOWN = 0,
    BH_FAULT_REASON_NOT_PRESENT,
    BH_FAULT_REASON_PROTECTION,
    BH_FAULT_REASON_ALIGNMENT,
    BH_FAULT_REASON_ILLEGAL_INSTRUCTION,
} bh_fault_reason_t;

/* Immutable, by-value semantic view decoded at the processor/kernel boundary.
 * The raw frame remains owner-local on the current CPU's kernel stack. */
typedef struct bh_trap_context {
    trap_class_t class_id;
    trap_origin_t origin;
    uintptr_t pc;
    uintptr_t sp;
    uintptr_t status;
    uintptr_t fault_addr;
    uint64_t arch_cause;
    bh_fault_access_t access;
    bh_fault_reason_t reason;
    bool interrupt_enabled;
} bh_trap_context_t;

typedef struct trap_info {
    trap_class_t  trap_class;
    trap_origin_t origin;

    uintptr_t     fault_addr;
    uintptr_t     ip;
    uintptr_t     sp;

    uint64_t      arch_code;
    uint64_t      error_code;
    int           vector;

    bool          write;
    bool          exec;
    bool          present;
    bool          recoverable;
    bool          interrupt_enabled;
} trap_info_t;
