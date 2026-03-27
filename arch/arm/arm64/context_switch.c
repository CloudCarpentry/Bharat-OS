#include "arch/context_switch.h"
#include "arch/arch_ext_state.h"
#include <stddef.h>
#include <stdint.h>


// Compile-time assertions for struct layout matching context_switch.S offsets
_Static_assert(__builtin_offsetof(cpu_context_t, regs) == 0, "cpu_context_t.regs offset mismatch");
_Static_assert(__builtin_offsetof(cpu_context_t, pc) == 128, "cpu_context_t.pc offset mismatch");
_Static_assert(__builtin_offsetof(cpu_context_t, sp) == 136, "cpu_context_t.sp offset mismatch");

extern void arch_kthread_start_trampoline(void);

void arch_prepare_initial_context(cpu_context_t* ctx, void (*entry)(void), uint64_t stack_top) {
  if (!ctx) {
    return;
  }
  for (size_t i = 0; i < 16; ++i) {
    ctx->regs[i] = 0U;
  }

  // Align stack to 16 bytes
  stack_top &= ~0xFULL;

  // x19 (regs[0]) will hold the real entry point
  ctx->regs[0] = (uint64_t)(uintptr_t)entry;

  // Initial resume point is the bootstrap trampoline
  ctx->pc = (uint64_t)(uintptr_t)arch_kthread_start_trampoline;
  ctx->sp = stack_top;

  // Do not preload FP state here. Lazy path will trap on first use.
}
