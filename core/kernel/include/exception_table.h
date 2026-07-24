#ifndef BHARAT_KERNEL_EXCEPTION_TABLE_H
#define BHARAT_KERNEL_EXCEPTION_TABLE_H

#include <stdint.h>
#include <stdbool.h>
#include "trap.h"

enum bh_ex_type {
    BH_EX_NONE       = 0,
    BH_EX_UACCESS    = 1,
    BH_EX_UACCESS_RD = 2,
    BH_EX_UACCESS_WR = 3,
};

struct bh_exception_entry {
    int32_t fault_off;
    int32_t fixup_off;
    uint16_t type;
    uint16_t data;
};

_Static_assert(sizeof(struct bh_exception_entry) == 12,
               "exception entry ABI drift");

static inline uintptr_t ex_fault_pc(const struct bh_exception_entry *e) {
    return (uintptr_t)&e->fault_off + e->fault_off;
}

static inline uintptr_t ex_fixup_pc(const struct bh_exception_entry *e) {
    return (uintptr_t)&e->fixup_off + e->fixup_off;
}

const struct bh_exception_entry *exception_table_lookup(uintptr_t pc);
bool trap_try_exception_fixup(trap_frame_t *tf, uintptr_t fault_addr, uint32_t fault_flags);

#endif /* BHARAT_KERNEL_EXCEPTION_TABLE_H */
