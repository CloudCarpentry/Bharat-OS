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

  // Align stack to 16 bytes per System V ABI
  stack_top &= ~0xFULL;

  // Push the thread exit trampoline address onto the stack.
  // When the entry function returns, it will "ret" into the trampoline.
  uint64_t* stack_ptr = (uint64_t*)(uintptr_t)stack_top;
  *(--stack_ptr) = (uint64_t)(uintptr_t)sched_thread_exit_trampoline;

  ctx->pc = (uint64_t)(uintptr_t)entry;
  ctx->sp = (uint64_t)(uintptr_t)stack_ptr;
}
