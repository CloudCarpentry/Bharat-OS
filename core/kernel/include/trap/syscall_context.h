#ifndef BHARAT_SYSCALL_CONTEXT_H
#define BHARAT_SYSCALL_CONTEXT_H

#include "trap/syscall_regs.h"

typedef long (*bh_syscall_handler_t)(bh_syscall_ctx_t *ctx);

typedef struct bh_syscall_desc {
    uint32_t nr;
    const char *name;
    uint32_t arg_count;
    uint64_t flags;
    uint64_t required_rights;
    bh_syscall_handler_t handler;
} bh_syscall_desc_t;

typedef struct bh_personality_syscall_table {
    const char *name;
    uint32_t abi_version;
    uint32_t max_syscall_nr; // Maximum valid syscall number (index)
    const bh_syscall_desc_t *table;
} bh_personality_syscall_table_t;

// Flags
#define BH_SYSCALL_F_FAST          (1u << 0)
#define BH_SYSCALL_F_BLOCKING      (1u << 1)
#define BH_SYSCALL_F_USER_READ     (1u << 2)
#define BH_SYSCALL_F_USER_WRITE    (1u << 3)
#define BH_SYSCALL_F_CAP_REQUIRED  (1u << 4)
#define BH_SYSCALL_F_SERVICE_CALL  (1u << 5)
#define BH_SYSCALL_F_COMPAT        (1u << 6)
#define BH_SYSCALL_F_AUDIT         (1u << 7)

#endif /* BHARAT_SYSCALL_CONTEXT_H */
