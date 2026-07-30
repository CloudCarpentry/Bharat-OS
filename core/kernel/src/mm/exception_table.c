#include "exception_table.h"
#include <stddef.h>

extern const char __ex_table_start[];
extern const char __ex_table_end[];

const struct bh_exception_entry *exception_table_lookup(uintptr_t pc) {
    const struct bh_exception_entry *start = (const struct bh_exception_entry *)__ex_table_start;
    const struct bh_exception_entry *end = (const struct bh_exception_entry *)__ex_table_end;

    for (const struct bh_exception_entry *entry = start; entry < end; entry++) {
        if (ex_fault_pc(entry) == pc) {
            return entry;
        }
    }
    return NULL;
}

bool trap_try_exception_fixup(trap_frame_t *tf, uintptr_t fault_addr, uint32_t fault_flags) {
    (void)fault_addr;
    if (!tf) return false;

    // If trap is from userspace, it is never a kernel exception-table fixup
    if (tf->from_user) {
        return false;
    }

    uintptr_t pc = tf->pc;
    const struct bh_exception_entry *ex = exception_table_lookup(pc);
    if (!ex) {
        return false;
    }

    // Validate the exception entry matches read/write if we have fault flags (e.g. read/write fault)
    // 1 for Write, 0 for Read/Execute
    if (ex->type == BH_EX_UACCESS_RD && (fault_flags & 1)) {
        return false;
    }
    if (ex->type == BH_EX_UACCESS_WR && !(fault_flags & 1)) {
        return false;
    }

    // Redirect the saved PC in the trap frame to the fixup handler
    tf->pc = ex_fixup_pc(ex);
    return true;
}
