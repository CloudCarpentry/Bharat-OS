#ifndef BHARAT_SYSCALL_CONTEXT_H
#define BHARAT_SYSCALL_CONTEXT_H

#include <stdint.h>
#include <stdbool.h>
#include "trap/syscall_regs.h"
#include "trap_types.h"

typedef enum bh_syscall_return_disposition {
    BH_SYSCALL_RETURN_USER = 0,
    BH_SYSCALL_RETURN_FAULT,
    BH_SYSCALL_RETURN_TERMINATE,
} bh_syscall_return_disposition_t;

/*
 * Normalized, by-value return state owned by the syscall trap path.  An
 * architecture may restore these fields only when disposition is USER.
 */
typedef struct bh_syscall_return_context {
    uintptr_t pc;
    uintptr_t sp;
    uintptr_t status;
    uintptr_t result;
    trap_origin_t origin;
    uint32_t flags;
    bh_syscall_return_disposition_t disposition;
} bh_syscall_return_context_t __attribute__((unused));

__attribute__((noreturn)) void
bh_syscall_rejected_return_handoff(bh_syscall_return_disposition_t disposition);

typedef long (*bh_syscall_handler_t)(bh_syscall_ctx_t *ctx);

/**
 * Syscall classification for auditing and policy enforcement.
 */
typedef enum bh_syscall_class {
    BH_SYS_CLASS_NONE = 0,
    BH_SYS_CLASS_PROCESS,
    BH_SYS_CLASS_MEMORY,
    BH_SYS_CLASS_IPC,
    BH_SYS_CLASS_IO,
    BH_SYS_CLASS_CAPABILITY,
    BH_SYS_CLASS_SYSTEM
} bh_syscall_class_t;

typedef enum bh_syscall_cap_source_kind {
    BH_SYS_CAP_SOURCE_NONE = 0,
    BH_SYS_CAP_SOURCE_REGISTER,
    BH_SYS_CAP_SOURCE_STRUCT_FIELD,
    BH_SYS_CAP_SOURCE_IMPLICIT_PROCESS,
    BH_SYS_CAP_SOURCE_IMPLICIT_THREAD
} bh_syscall_cap_source_kind_t;

typedef enum bh_syscall_cap_val_phase {
    BH_SYS_CAP_VAL_NONE = 0,
    BH_SYS_CAP_VAL_BEFORE_HANDLER,
    BH_SYS_CAP_VAL_AFTER_USERCOPY
} bh_syscall_cap_val_phase_t;

/**
 * Production-grade Syscall Metadata
 */
typedef struct bh_syscall_meta {
    uint32_t nr;
    const char *name;
    bh_syscall_class_t class_id;
    uint32_t arg_count;
    uint64_t flags;
    uint64_t required_rights;
    uint8_t  cap_arg_index;
    uint32_t required_cap_type;
    bh_syscall_cap_source_kind_t cap_source_kind;
    const char *cap_source_field;
    bh_syscall_cap_val_phase_t cap_val_phase;
    bh_syscall_handler_t handler;
} bh_syscall_meta_t;

typedef struct bh_personality_syscall_table {
    const char *name;
    uint32_t abi_version;
    uint32_t entry_count; // Entry count of the syscall table
    const bh_syscall_meta_t *table;
} bh_personality_syscall_table_t;

#include "kernel/status.h"
kstatus_t bh_syscall_table_validate(const bh_personality_syscall_table_t *table);

// Flags
#define BH_SYSCALL_F_FAST          (1u << 0)
#define BH_SYSCALL_F_BLOCKING      (1u << 1)
#define BH_SYSCALL_F_USER_READ     (1u << 2)
#define BH_SYSCALL_F_USER_WRITE    (1u << 3)
#define BH_SYSCALL_F_CAP_REQUIRED  (1u << 4)
#define BH_SYSCALL_F_SERVICE_CALL  (1u << 5)
#define BH_SYSCALL_F_COMPAT        (1u << 6)
#define BH_SYSCALL_F_AUDIT         (1u << 7)

#define BH_SYS_CAP_INDEX_NONE UINT8_MAX

#endif /* BHARAT_SYSCALL_CONTEXT_H */
