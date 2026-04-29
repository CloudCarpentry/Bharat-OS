#ifndef BHARAT_SYSCALL_CONTEXT_H
#define BHARAT_SYSCALL_CONTEXT_H

#include <stdint.h>
#include <stdbool.h>
#include "trap/syscall_regs.h"

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
    bool requires_capability;
    bool copies_from_user;
    bool copies_to_user;
    bh_syscall_handler_t handler;
} bh_syscall_meta_t;

/* For backward compatibility during transition */
typedef bh_syscall_meta_t bh_syscall_desc_t;

typedef struct bh_personality_syscall_table {
    const char *name;
    uint32_t abi_version;
    uint32_t max_syscall_nr; // Maximum valid syscall number (index)
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
