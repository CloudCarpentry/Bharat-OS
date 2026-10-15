#include "arch/user_entry.h"
#include "kernel/status.h"
#include "mm/prot_domain.h"
#include "panic.h"
#include "console/console_core.h"

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

static void print_hex(uint64_t val) {
    char buf[17];
    for (int i = 15; i >= 0; --i) {
        int nibble = (val >> (i * 4)) & 0xF;
        buf[15 - i] = nibble < 10 ? '0' + nibble : 'a' + (nibble - 10);
    }
    buf[16] = '\0';
    console_write_raw(buf, 16);
}

__attribute__((noreturn))
void arch_enter_user(const arch_user_entry_t *entry) {
    if (!entry || !entry->aspace || !entry->aspace->prot_domain) {
        kernel_panic("arch_enter_user: invalid entry or aspace");
    }

    console_write_raw("[ARCH_ENTER_USER_ARM64]\n", 25);
    uint64_t cur_el;
    __asm__ volatile("mrs %0, CurrentEL" : "=r"(cur_el));
    cur_el >>= 2;
    console_write_raw("  CurrentEL=", 12);
    print_hex(cur_el);
    console_write_raw(" pc=", 5);
    print_hex(entry->entry_pc);
    console_write_raw(" sp=", 4);
    print_hex(entry->user_sp);
    console_write_raw(" arg0=", 6);
    print_hex(entry->arg0);
    console_write_raw("\n", 1);

    int act_rc = prot_domain_activate(entry->aspace->prot_domain);
    if (act_rc != K_OK) {
        console_write_raw("  prot_domain_activate failed rc=", 33);
        print_hex((uint64_t)act_rc);
        console_write_raw("\n", 1);
        kernel_panic("arch_enter_user: prot_domain_activate failed");
    }

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
