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



extern void x86_64_thread_entry_stub(void);

void arch_prepare_initial_context_arg(
    cpu_context_t *ctx,
    arch_thread_entry_arg_t entry,
    void *arg0,
    uintptr_t stack_top)
{
    if (!ctx || !entry) {
        return;
    }

    // memset ctx
    uint8_t *p = (uint8_t *)ctx;
    for (size_t i = 0; i < sizeof(*ctx); ++i) {
        p[i] = 0;
    }

    uintptr_t aligned_top = stack_top & ~(uintptr_t)0xFULL;
    uint64_t *sp = (uint64_t *)aligned_top;

    *(--sp) = (uint64_t)(uintptr_t)sched_thread_exit_trampoline;
    *(--sp) = (uint64_t)(uintptr_t)entry;
    *(--sp) = (uint64_t)(uintptr_t)arg0;

    ctx->pc = (uint64_t)(uintptr_t)x86_64_thread_entry_stub;
    ctx->sp = (uint64_t)(uintptr_t)sp;

    // RFLAGS reserved bit. IF is enabled at the controlled boundary.
    ctx->regs[6] = 0x2U;
}

_Static_assert(__builtin_offsetof(cpu_context_t, pc) == 128, "x86 context pc offset changed");
_Static_assert(__builtin_offsetof(cpu_context_t, sp) == 136, "x86 context sp offset changed");
