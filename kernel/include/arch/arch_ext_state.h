#ifndef BHARAT_ARCH_EXT_STATE_H
#define BHARAT_ARCH_EXT_STATE_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

struct kthread;

typedef struct arch_ext_state arch_ext_state_t;

typedef enum {
    ARCH_EXT_NONE        = 0,
    ARCH_EXT_FP_SIMD     = 1u << 0,
    ARCH_EXT_VECTOR      = 1u << 1,
    ARCH_EXT_SUPERVISOR  = 1u << 2,
} arch_ext_state_flags_t;

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

int arch_ext_state_thread_init(struct kthread *t);
void arch_ext_state_thread_destroy(struct kthread *t);

void arch_ext_state_save(struct kthread *t);
void arch_ext_state_restore(struct kthread *t);

void arch_kernel_fpu_begin(void);
void arch_kernel_fpu_end(void);

#endif // BHARAT_ARCH_EXT_STATE_H
