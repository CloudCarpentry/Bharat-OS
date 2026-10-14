#include "arch/user_entry.h"
#include "kernel/status.h"
#include "mm/prot_domain.h"
#include "panic.h"
#include "bharat/cpu_local.h"


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

    if (prot_domain_activate(entry->aspace->prot_domain) != K_OK) {
        kernel_panic("arch_enter_user: MPU/PMP protection activation failed");
    }

#if defined(BHARAT_RISCV_PRIV_MODE_M)
    uint32_t mstatus;
    asm volatile("csrr %0, mstatus" : "=r"(mstatus));

    // Clear MPP (User mode), set MPIE (interrupts enabled in user mode)
    // MPP is bits 11:12
    mstatus &= ~(3 << 11);
    mstatus |= (1 << 7); // MPIE

    extern uint32_t hal_cpu_get_id(void);
    uint32_t core = hal_cpu_get_id();
    uint32_t kstack = (uint32_t)g_cpu_locals[core].kernel_stack;
    asm volatile("csrw mscratch, %0" :: "r"(kstack));

    asm volatile(
        "csrw mstatus, %0\n\t"
        "csrw mepc, %1\n\t"
        "mv sp, %2\n\t"
        "mv a0, %3\n\t"
        "mret\n\t"
        :
        : "r"(mstatus), "r"(entry->entry_pc), "r"(entry->user_sp), "r"(entry->arg0)
        : "memory"
    );
#else
    uint32_t sstatus;
    asm volatile("csrr %0, sstatus" : "=r"(sstatus));

    // Clear SPP (User mode), set SPIE (interrupts enabled in user mode), clear SUM
    sstatus &= ~(SSTATUS_SPP | SSTATUS_SUM);
    sstatus |= SSTATUS_SPIE;

    extern uint32_t hal_cpu_get_id(void);
    uint32_t core = hal_cpu_get_id();
    uint32_t kstack = (uint32_t)g_cpu_locals[core].kernel_stack;
    asm volatile("csrw sscratch, %0" :: "r"(kstack));


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
#endif

    __builtin_unreachable();
}

