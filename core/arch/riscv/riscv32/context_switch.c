#include "arch/context_switch.h"
#include "arch/arch_ext_state.h"
#include <stddef.h>
#include <stdint.h>

extern void riscv_thread_entry_stub(void);

void arch_prepare_initial_context(cpu_context_t* ctx, void (*entry)(void), uint64_t stack_top) {
    if (!ctx) return;
    for (size_t i = 0; i < 16; ++i) ctx->regs[i] = 0U;
    stack_top &= ~0xFULL;
    ctx->regs[0] = (uintptr_t)sched_thread_exit_trampoline;
    ctx->regs[1] = 0U;
    ctx->regs[2] = (uintptr_t)entry;
    uint32_t sstatus_val;
    __asm__ volatile("csrr %0, sstatus" : "=r"(sstatus_val));
    ctx->regs[12] = sstatus_val;
    ctx->pc = (uintptr_t)riscv_thread_entry_stub;
    ctx->sp = (uintptr_t)stack_top;
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

    stack_top &= ~0xFU;

    ctx->regs[0] = (uintptr_t)arg0;  // s0 = arg0
    ctx->regs[1] = (uintptr_t)entry; // s1 = entry
    ctx->regs[2] = (uintptr_t)sched_thread_exit_trampoline; // s2


    uint32_t sstatus_val;
    __asm__ volatile("csrr %0, sstatus" : "=r"(sstatus_val));
    ctx->regs[12] = sstatus_val;

    ctx->pc = (uintptr_t)riscv_thread_entry_stub;
    ctx->sp = stack_top;
}


