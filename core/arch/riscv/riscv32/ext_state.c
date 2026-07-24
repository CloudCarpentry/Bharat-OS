#include "../../include/arch/arch_ext_state.h"
#include <stdbool.h>
#include <stddef.h>

const arch_ext_state_desc_t *arch_ext_state_desc(void) { return NULL; }
bool arch_ext_state_enabled(void) { return false; }
void arch_ext_state_boot_init(void) {}
int arch_ext_state_thread_init(struct bh_thread *t) { (void)t; return 0; }
void arch_ext_state_thread_destroy(struct bh_thread *t) { (void)t; }
void arch_ext_state_context_switch_out(void *prev_ctx) { (void)prev_ctx; }
void arch_ext_state_context_switch_in(void *next_ctx) { (void)next_ctx; }
bool arch_ext_state_handle_fault(struct bh_thread *t) { (void)t; return false; }
void arch_ext_state_save(struct bh_thread *t) { (void)t; }
void arch_ext_state_restore(struct bh_thread *t) { (void)t; }
void arch_kernel_fpu_begin(void) {}
void arch_kernel_fpu_end(void) {}
