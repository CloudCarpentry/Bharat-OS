#include "arch/user_entry.h"

kstatus_t arch_user_entry_prepare(
    arch_user_entry_t *out,
    address_space_t *aspace,
    uintptr_t entry_pc,
    uintptr_t user_sp,
    uintptr_t arg0)
{
    return K_ERR_UNSUPPORTED;
}

__attribute__((noreturn))
void arch_enter_user(const arch_user_entry_t *entry) {
    while (1) {}
}
