#include "arch/context_switch.h"
#include <stddef.h>
#include <stdint.h>

void arch_context_switch(cpu_context_t* prev, cpu_context_t* next) {
  if (prev) {
    __asm__ volatile("" ::: "memory");
  }
  if (next) {
    __asm__ volatile("" ::: "memory");
  }
}

void arch_prepare_initial_context(cpu_context_t* ctx, void (*entry)(void), uint64_t stack_top) {
  if (!ctx) {
    return;
  }
  for (size_t i = 0; i < 16; ++i) {
    ctx->regs[i] = 0U;
  }
  ctx->pc = (uint64_t)(uintptr_t)entry;
  ctx->sp = stack_top;
}
