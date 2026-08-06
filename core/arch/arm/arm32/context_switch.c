#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

void arch_context_switch(void *prev_state, void *next_state) { (void)prev_state; (void)next_state; }
void arch_prepare_initial_context(void *thread, void *entry, void *stack_top, void *arg) {
    (void)thread; (void)entry; (void)stack_top; (void)arg;
}
void arch_ext_state_thread_init(void *thread) { (void)thread; }
void arch_ext_state_thread_destroy(void *thread) { (void)thread; }
void arch_ext_state_save(void *thread) { (void)thread; }
void arch_ext_state_restore(void *thread) { (void)thread; }

void arch_prepare_initial_context_arg(
    cpu_context_t *ctx,
    arch_thread_entry_arg_t entry,
    void *arg0,
    uintptr_t stack_top)
{
    // Stub
}
