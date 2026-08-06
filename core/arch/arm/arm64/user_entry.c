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
        "mov x0, %0\n\t"
        "msr elr_el1, %1\n\t"
        "msr sp_el0, %2\n\t"
        "mov x3, #0\n\t"
        "msr spsr_el1, x3\n\t"
        "eret\n\t"
        :
        : "r"(entry->arg0), "r"(entry->entry_pc), "r"(entry->user_sp)
        : "x0", "x3", "memory"
    );
    while (1) {}
}
