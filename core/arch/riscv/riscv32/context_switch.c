#include "arch/context_switch.h"
#include "arch/arch_ext_state.h"
#include <stddef.h>
#include <stdint.h>

void arch_prepare_initial_context(cpu_context_t* ctx, void (*entry)(void), uint64_t stack_top) {
    (void)ctx;
    (void)entry;
    (void)stack_top;
    // riscv32 context switch is currently a stub
}

void arch_prepare_initial_context_arg(
    cpu_context_t *ctx,
    arch_thread_entry_arg_t entry,
    void *arg0,
    uintptr_t stack_top)
{
    if (!ctx) {
        return;
    }
    for (size_t i = 0; i < 16; ++i) {
        ctx->regs[i] = 0U;
    }

    // Align stack to 16 bytes
    stack_top &= ~0xF;

    // ra (return address) goes into regs[0] as per context save/restore convention
    ctx->regs[0] = (uintptr_t)sched_thread_exit_trampoline;

    /*
     * regs[12] backs saved sstatus in the current layout.
     * Start with FS=Off for lazy trap-on-first-use.
     */
    ctx->regs[12] = 0U;

    // a0 is x10, but in context_switch we typically only save callee-saved registers.
    // However, context_switch.S in RISC-V usually expects arguments to be passed directly.
    // Wait, the generic initial context arg setup uses s0/s1 for arg passing.
    // Let's set a0 directly if it's in the generic cpu_context_t, but cpu_context_t only holds callee-saved.
    // Let's assume s1 (regs[2]) holds entry and s0 (regs[1]) holds arg0.

    ctx->pc = (uintptr_t)entry;
    ctx->sp = stack_top;

    // RISC-V typical arg setup:
    ctx->regs[1] = (uintptr_t)arg0; // s0
    ctx->regs[2] = (uintptr_t)entry; // s1
}
