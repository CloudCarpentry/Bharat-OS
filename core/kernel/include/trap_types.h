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
