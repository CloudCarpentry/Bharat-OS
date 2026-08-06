#ifndef BHARAT_ARCH_CONTEXT_SWITCH_H
#define BHARAT_ARCH_CONTEXT_SWITCH_H

#include "sched/sched.h"

void arch_context_switch(cpu_context_t* prev, cpu_context_t* next);
void arch_prepare_initial_context(cpu_context_t* ctx, void (*entry)(void), uint64_t stack_top);
void sched_thread_exit_trampoline(void);

typedef void (*arch_thread_entry_arg_t)(void *arg0);

void arch_prepare_initial_context_arg(
    cpu_context_t *ctx,
    arch_thread_entry_arg_t entry,
    void *arg0,
    uintptr_t stack_top);


#endif
