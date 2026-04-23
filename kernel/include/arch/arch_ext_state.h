#ifndef BHARAT_ARCH_EXT_STATE_H
#define BHARAT_ARCH_EXT_STATE_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

struct bh_thread;

typedef struct arch_ext_state arch_ext_state_t;

typedef enum {
    ARCH_EXT_NONE        = 0,
    ARCH_EXT_FP_SIMD     = 1u << 0,
    ARCH_EXT_VECTOR      = 1u << 1,
    ARCH_EXT_SUPERVISOR  = 1u << 2,
} arch_ext_state_flags_t;

typedef enum {
    ARCH_EXT_STATE_VALID        = 1u << 0,
    ARCH_EXT_STATE_LOADED       = 1u << 1,
    ARCH_EXT_STATE_DIRTY        = 1u << 2,
    ARCH_EXT_STATE_TRAP_ENABLED = 1u << 3,
} arch_ext_state_state_flags_t;

typedef struct arch_ext_state_desc {
    size_t size;
    size_t align;
    uint32_t flags;
    bool eager;
    bool lazy_allowed;
} arch_ext_state_desc_t;

void arch_ext_state_boot_init(void);
const arch_ext_state_desc_t *arch_ext_state_desc(void);
bool arch_ext_state_enabled(void);

int arch_ext_state_thread_init(struct bh_thread *t);
void arch_ext_state_thread_destroy(struct bh_thread *t);

void arch_ext_state_save(struct bh_thread *t);
void arch_ext_state_restore(struct bh_thread *t);

void arch_ext_state_context_switch_out(void *prev_ctx);
void arch_ext_state_context_switch_in(void *next_ctx);

/* called from trap path when FP/SIMD use traps */
bool arch_ext_state_handle_fault(struct bh_thread *t);

void arch_kernel_fpu_begin(void);
void arch_kernel_fpu_end(void);

#endif // BHARAT_ARCH_EXT_STATE_H
