#include "arch/context_switch.h"

void arch_context_switch(cpu_context_t *prev, cpu_context_t *next) {
  (void)prev;
  (void)next;
}

void arch_prepare_initial_context(cpu_context_t *ctx, void (*entry)(void),
                                  uint64_t stack_top) {
  (void)ctx;
  (void)entry;
  (void)stack_top;
}

extern void arch_bh_thread_start_trampoline(void);

void arch_prepare_initial_context_arg(cpu_context_t *ctx,
                                      arch_thread_entry_arg_t entry, void *arg0,
                                      uintptr_t stack_top) {
  if (!ctx) return;
  for (int i = 0; i < 16; i++) {
    ctx->regs[i] = 0;
  }

  stack_top &= ~0x7ULL;

  ctx->regs[4] = (uintptr_t)entry;
  ctx->regs[0] = (uintptr_t)arg0;

  ctx->pc = (uintptr_t)arch_bh_thread_start_trampoline;
  ctx->sp = stack_top;
}
