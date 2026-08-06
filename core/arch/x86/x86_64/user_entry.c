#include "arch/user_entry.h"
#include "console/console_core.h"
#include "hal/hal.h"
#include "panic.h"
#include "sched/sched.h"
#include "mm/physmap.h"

kstatus_t arch_user_entry_prepare(
    arch_user_entry_t *out,
    address_space_t *aspace,
    uintptr_t entry_pc,
    uintptr_t user_sp,
    uintptr_t arg0)
{
    if (!out || !aspace) return K_ERR_INVALID_ARG;

    // Canonical address check (simplified)
    if ((entry_pc >> 47) != 0 && (entry_pc >> 47) != 0x1FFFF) {
        return K_ERR_INVALID_ARG;
    }
    if ((user_sp >> 47) != 0 && (user_sp >> 47) != 0x1FFFF) {
        return K_ERR_INVALID_ARG;
    }

    out->entry_pc = entry_pc;
    out->user_sp = user_sp;
    out->arg0 = arg0;
    out->aspace = aspace;
    // out->flags preserved
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
    uint64_t cr3_val;
    __asm__ volatile("mov %%cr3, %0" : "=r"(cr3_val));

    bh_thread_t *thread = sched_current_thread();

    // Attempt to read TSS rsp0 (assuming CPU 0 for now)


    console_write_raw("USER_ENTRY_X86:\n", 16);

    console_write_raw("  rip=", 6);
    print_hex(entry->entry_pc);
    console_write_raw("\n  rsp=", 7);
    print_hex(entry->user_sp);
    console_write_raw("\n  arg0=", 8);
    print_hex(entry->arg0);
    console_write_raw("\n  cs=0x23\n  ss=0x1b\n  rflags=0x202\n", 36);

    console_write_raw("  cr3_actual=", 13);
    print_hex(cr3_val);
    console_write_raw("\n  cr3_expected=", 16);
    print_hex(0);
    console_write_raw("\n  pid=", 7);
    print_hex(thread ? thread->process_id : 0);
    console_write_raw("\n  tid=", 7);
    print_hex(thread ? thread->thread_id : 0);
    console_write_raw("\n  cpu=", 7);
    print_hex(hal_cpu_get_id());
    console_write_raw("\n", 1);

    __asm__ volatile (
        "cli\n\t"
        "movq %0, %%rdi\n\t"
        "movq $0x1B, %%rax\n\t"
        "movq %%rax, %%ds\n\t"
        "movq %%rax, %%es\n\t"
        "pushq $0x1B\n\t"
        "pushq %1\n\t"
        "pushq $0x202\n\t"
        "pushq $0x23\n\t"
        "pushq %2\n\t"
        "iretq\n\t"
        :
        : "r"(entry->arg0), "r"(entry->user_sp), "r"(entry->entry_pc)
        : "rax", "rdi", "memory"
    );
    while (1) {}
}
