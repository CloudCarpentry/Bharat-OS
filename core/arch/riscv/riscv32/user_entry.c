#include "arch/user_entry.h"
#include "kernel/status.h"
#include "mm/prot_domain.h"
#include "panic.h"

#define SSTATUS_SPP (1 << 8)
#define SSTATUS_SPIE (1 << 5)
#define SSTATUS_SUM (1 << 18)

kstatus_t arch_user_entry_prepare(
    arch_user_entry_t *out,
    address_space_t *aspace,
    uintptr_t entry_pc,
    uintptr_t user_sp,
    uintptr_t arg0)
{
    if (!out || !aspace) return K_ERR_INVALID_ARG;
    if (entry_pc == 0 || user_sp == 0) return K_ERR_INVALID_ARG;
    if ((user_sp & 0xF) != 0) return K_ERR_INVALID_ARG; // RISC-V ABI requires 16-byte aligned stack

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

    uint32_t sstatus;
    asm volatile("csrr %0, sstatus" : "=r"(sstatus));

    // Clear SPP (User mode), set SPIE (interrupts enabled in user mode), clear SUM
    sstatus &= ~(SSTATUS_SPP | SSTATUS_SUM);
    sstatus |= SSTATUS_SPIE;

    asm volatile(
        "csrw sstatus, %0\n\t"
        "csrw sepc, %1\n\t"
        "mv sp, %2\n\t"
        "mv a0, %3\n\t"
        "sret\n\t"
        :
        : "r"(sstatus), "r"(entry->entry_pc), "r"(entry->user_sp), "r"(entry->arg0)
        : "memory"
    );

    __builtin_unreachable();
}
