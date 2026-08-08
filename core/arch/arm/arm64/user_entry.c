#include "arch/user_entry.h"
#include "kernel/status.h"
#include "mm/prot_domain.h"
#include "panic.h"

kstatus_t arch_user_entry_prepare(
    arch_user_entry_t *out,
    address_space_t *aspace,
    uintptr_t entry_pc,
    uintptr_t user_sp,
    uintptr_t arg0)
{
    if (!out || !aspace) return K_ERR_INVALID_ARG;
    if (entry_pc == 0 || user_sp == 0) return K_ERR_INVALID_ARG;
    if ((user_sp & 0xF) != 0) return K_ERR_INVALID_ARG; // ARM64 ABI requires 16-byte aligned stack
    if ((entry_pc & 0x3) != 0) return K_ERR_INVALID_ARG; // ARM64 instructions are 4-byte aligned

    out->entry_pc = entry_pc;
    out->user_sp = user_sp;
    out->arg0 = arg0;
    out->aspace = aspace;
    return K_OK;
}

__attribute__((noreturn))
void arch_enter_user(const arch_user_entry_t *entry) {
    if (!entry || !entry->aspace || !entry->aspace->prot_domain) {
        kernel_panic("arch_enter_user: invalid entry or aspace");
    }

    prot_domain_activate(entry->aspace->prot_domain);

    // SPSR_EL1 configuration for EL0t:
    // M[4:0] = 0b00000 (EL0t)
    // DAIF = 0b0000 (interrupts enabled)
    // NZCV = 0 (flags cleared)
    uint64_t spsr = 0x0;

    __asm__ volatile (
        "msr elr_el1, %1\n\t"
        "msr sp_el0, %2\n\t"
        "msr spsr_el1, %3\n\t"
        "mov x0, %0\n\t"
        "eret\n\t"
        :
        : "r"(entry->arg0), "r"(entry->entry_pc), "r"(entry->user_sp), "r"(spsr)
        : "x0", "memory"
    );
    __builtin_unreachable();
}
