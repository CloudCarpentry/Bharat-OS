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

  // ra (return address) goes into regs[0] as per context save/restore convention
  ctx->regs[0] = (uint64_t)(uintptr_t)sched_thread_exit_trampoline;

  // regs[12] (offset 96) is used for sstatus in context_switch.S
  // Set FS (Floating-point Status) to 1 (Initial) so fld/fsd instructions work
  // FS is bits [14:13] in sstatus. 1 << 13 = 0x2000
  ctx->regs[12] = 0x2000;

  ctx->pc = (uint64_t)(uintptr_t)entry;
  ctx->sp = stack_top;
}
