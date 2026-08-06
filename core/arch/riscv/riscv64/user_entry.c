#include "arch/user_entry.h"

kstatus_t arch_user_entry_prepare(
    arch_user_entry_t *out,
    address_space_t *aspace,
    uintptr_t entry_pc,
    uintptr_t user_sp,
    uintptr_t arg0)
{
    if (!out) return K_ERR_INVALID_ARG;
    out->entry_pc = entry_pc;
    out->user_sp = user_sp;
    out->arg0 = arg0;
    out->aspace = aspace;
    return K_OK;
}

__attribute__((noreturn))
void arch_enter_user(const arch_user_entry_t *entry) {
    __asm__ volatile (
        "mv a0, %0\n\t"
        "csrw sepc, %1\n\t"
        "mv sp, %2\n\t"
        "sret\n\t"
        :
        : "r"(entry->arg0), "r"(entry->entry_pc), "r"(entry->user_sp)
        : "a0", "sp", "memory"
    );
    while (1) {}
}
