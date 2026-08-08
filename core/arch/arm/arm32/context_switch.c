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

void arch_prepare_initial_context_arg(cpu_context_t *ctx,
                                      arch_thread_entry_arg_t entry, void *arg0,
                                      uintptr_t stack_top) {
  (void)ctx;
  (void)entry;
  (void)arg0;
  (void)stack_top;
}
