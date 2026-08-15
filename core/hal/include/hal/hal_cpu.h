#pragma once

#include <stdint.h>

/* Opaque architecture interrupt mask snapshot, restored only on this CPU. */
typedef uintptr_t hal_irq_state_t;

struct hal_cpu_info {
    uint32_t logical_id;
    uint32_t cluster_id;
    uint32_t flags;
};

int hal_cpu_current(void);
const struct hal_cpu_info *hal_cpu_info(uint32_t cpu_id);

/* Save the local mask and disable interrupts; paired restores preserve nesting. */
hal_irq_state_t hal_irq_save_disable(void);
void hal_irq_restore(hal_irq_state_t state);
