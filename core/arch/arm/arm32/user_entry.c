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

    // Check ARM alignment rules (EABI requires 8-byte aligned stack, PC must be 4-byte aligned for ARM, 2 for Thumb)
    // We assume ARM instruction set for entry PC for now.
    if ((user_sp & 0x7) != 0) return K_ERR_INVALID_ARG;

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

    uint32_t user_cpsr = 0x10; // USR mode (0b10000), IRQ and FIQ enabled (I=0, F=0)

    // Check if entry_pc is thumb
    if (entry->entry_pc & 1) {
        user_cpsr |= (1 << 5); // T bit
    }

    // Set User SP and LR using CPS to System mode (or by accessing them directly if supported, but CPS is standard in ARMv7)
    // Wait, QEMU ARM32 virt is typically ARMv7-A.
    // Instead of switching modes which can be tricky in C inline asm, we can just use the banked registers via ldm ^
    // But easiest is to set SPSR, LR, switch to System mode, set SP, switch back to SVC, then movs pc, lr.

    asm volatile(
        "msr spsr, %3\n\t"        /* Set SPSR to user CPSR */
        "mov lr, %2\n\t"          /* Set SVC LR to entry_pc */
        "cps #0x1F\n\t"           /* Switch to System mode (shares SP with User mode) */
        "mov sp, %1\n\t"          /* Set User SP */
        "cps #0x13\n\t"           /* Switch back to SVC mode */
        "mov r0, %0\n\t"          /* Set r0 to arg0 */
        "movs pc, lr\n\t"         /* Return from exception to User mode */
        :
        : "r"(entry->arg0), "r"(entry->user_sp), "r"(entry->entry_pc), "r"(user_cpsr)
        : "r0", "lr", "memory"
    );

    __builtin_unreachable();
}
