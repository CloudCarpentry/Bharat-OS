#include "arch/context_switch.h"
#include <stddef.h>
#include <stdint.h>


void arch_prepare_initial_context(cpu_context_t* ctx, void (*entry)(void), uint64_t stack_top) {
  if (!ctx) {
    return;
  }
  for (size_t i = 0; i < 16; ++i) {
    ctx->regs[i] = 0U;
  }

  // Align stack to 16 bytes
  stack_top &= ~0xFULL;

  // Set link register (x30) to point to the exit trampoline.
  // We'll store it in ctx->regs[15] as per our convention (or depending on context save/restore logic)
  ctx->regs[15] = (uint64_t)(uintptr_t)sched_thread_exit_trampoline;

  ctx->pc = (uint64_t)(uintptr_t)entry;
  ctx->sp = stack_top;
}
