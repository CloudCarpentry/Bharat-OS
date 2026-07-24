#include "arch/context_switch.h"
#include "arch/arch_ext_state.h"
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

  // ra (return address) goes into regs[0] as per context save/restore convention
  ctx->regs[0] = (uint64_t)(uintptr_t)sched_thread_exit_trampoline;

  /*
   * regs[12] backs saved sstatus in the current layout.
   * Start with FS=Off for lazy trap-on-first-use.
   */
  ctx->regs[12] = 0ULL;

  ctx->pc = (uint64_t)(uintptr_t)entry;
  ctx->sp = stack_top;
}
