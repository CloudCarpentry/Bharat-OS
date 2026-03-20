#include "hal/hal_irq.h"
#include "kernel_safety.h"
#include "spinlock.h"

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define HAL_MAX_IRQS 256U
#define HAL_MAX_SHARED_HANDLERS 4U

// --- IRQ Handler Descriptor ---
typedef struct irq_action {
    hal_irq_handler_t handler;
    void* ctx;
    void* dev_id;
    const char* name;
    uint32_t flags;
    uint64_t dispatch_count;
    bool in_use;
} irq_action_t;

// --- IRQ Descriptor ---
typedef struct irq_desc {
    uint32_t irq;
    uint32_t flags;        // Cumulative flags for the descriptor (e.g., IRQF_SHARED)
    uint64_t dispatch_count;
    uint64_t handled_count;
    uint64_t spurious_count;

    // Affinity/Routing metadata
    irq_affinity_mask_t affinity;
    uint32_t last_handled_cpu;

    // Fixed array of handlers for simplicity; could use linked list if needed
    irq_action_t actions[HAL_MAX_SHARED_HANDLERS];
    uint32_t action_count;

    // Add lock (using atomic variable for simple lock-free sync if spinlocks are complex, but let's assume spinlock_t exists)
    spinlock_t lock;
} irq_desc_t;

static irq_desc_t g_irq_descriptors[HAL_MAX_IRQS];

void hal_irq_generic_init_boot(void) {
    for (uint32_t i = 0; i < HAL_MAX_IRQS; i++) {
        g_irq_descriptors[i].irq = i;
        g_irq_descriptors[i].flags = 0;
        g_irq_descriptors[i].dispatch_count = 0;
        g_irq_descriptors[i].handled_count = 0;
        g_irq_descriptors[i].spurious_count = 0;
        g_irq_descriptors[i].action_count = 0;
        g_irq_descriptors[i].affinity.mask = ~0ULL; // Default to all online CPUs
        g_irq_descriptors[i].last_handled_cpu = (uint32_t)-1;

        spin_lock_init(&g_irq_descriptors[i].lock);

        for (uint32_t j = 0; j < HAL_MAX_SHARED_HANDLERS; j++) {
            g_irq_descriptors[i].actions[j].in_use = false;
        }
    }
}

int hal_interrupt_register(uint32_t irq, hal_irq_handler_t handler, void* ctx, uint32_t flags, const char* name, void* dev_id) {
    if (irq >= HAL_MAX_IRQS || !handler || !dev_id) {
        return -1;
    }

    irq_desc_t* desc = &g_irq_descriptors[irq];

    spin_lock(&desc->lock);

    // If IRQ already has handlers, check sharing rules
    if (desc->action_count > 0) {
        // Can only share if BOTH the existing line is shared AND the new request is shared
        if (!(desc->flags & IRQF_SHARED) || !(flags & IRQF_SHARED)) {
            spin_unlock(&desc->lock);
            return -1; // Registration rejected
        }
    }

    // Check for duplicate dev_id
    for (uint32_t i = 0; i < HAL_MAX_SHARED_HANDLERS; i++) {
        if (desc->actions[i].in_use && desc->actions[i].dev_id == dev_id) {
            spin_unlock(&desc->lock);
            return -1; // Duplicate dev_id
        }
    }

    // Find an empty slot
    int slot = -1;
    for (uint32_t i = 0; i < HAL_MAX_SHARED_HANDLERS; i++) {
        if (!desc->actions[i].in_use) {
            slot = i;
            break;
        }
    }

    if (slot == -1) {
        spin_unlock(&desc->lock);
        return -1; // No more slots
    }

    desc->actions[slot].handler = handler;
    desc->actions[slot].ctx = ctx;
    desc->actions[slot].dev_id = dev_id;
    desc->actions[slot].name = name;
    desc->actions[slot].flags = flags;
    desc->actions[slot].dispatch_count = 0;
    desc->actions[slot].in_use = true;

    desc->action_count++;
    if (desc->action_count == 1) {
        desc->flags = flags; // First handler sets the baseline flags
    } else {
        desc->flags |= flags;
    }

    spin_unlock(&desc->lock);
    return 0;
}

int hal_interrupt_unregister(uint32_t irq, void* dev_id) {
    if (irq >= HAL_MAX_IRQS || !dev_id) {
        return -1;
    }

    irq_desc_t* desc = &g_irq_descriptors[irq];
    int status = -1;

    spin_lock(&desc->lock);

    for (uint32_t i = 0; i < HAL_MAX_SHARED_HANDLERS; i++) {
        if (desc->actions[i].in_use && desc->actions[i].dev_id == dev_id) {
            desc->actions[i].in_use = false;
            desc->actions[i].handler = NULL;
            desc->actions[i].ctx = NULL;
            desc->actions[i].dev_id = NULL;
            desc->actions[i].name = NULL;
            desc->actions[i].flags = 0;

            desc->action_count--;
            if (desc->action_count == 0) {
                desc->flags = 0;
            }

            status = 0;
            break;
        }
    }

    spin_unlock(&desc->lock);
    return status;
}

void hal_interrupt_dispatch(uint32_t irq) {
    if (irq >= HAL_MAX_IRQS) {
        return;
    }

    irq_desc_t* desc = &g_irq_descriptors[irq];

    // In a real dispatch, you wouldn't typically lock if the IRQ context is non-preemptible,
    // but for shared lists that can be modified, a brief lock or RCU-like protection is needed.
    // For simplicity, we use spin_lock.
    spin_lock(&desc->lock);

    desc->dispatch_count++;

    bool handled = false;
    for (uint32_t i = 0; i < HAL_MAX_SHARED_HANDLERS; i++) {
        if (desc->actions[i].in_use && desc->actions[i].handler) {
            irq_return_t ret = desc->actions[i].handler(desc->actions[i].ctx);
            desc->actions[i].dispatch_count++;
            if (ret == IRQ_HANDLED || ret == IRQ_WAKE_DEFERRED) {
                handled = true;
            }
        }
    }

    if (handled) {
        desc->handled_count++;
    } else {
        desc->spurious_count++;
    }

    spin_unlock(&desc->lock);
}

uint64_t hal_interrupt_get_dispatch_count(uint32_t irq) {
    if (irq >= HAL_MAX_IRQS) {
        return 0U;
    }

    uint64_t count = 0;
    irq_desc_t* desc = &g_irq_descriptors[irq];

    spin_lock(&desc->lock);
    count = desc->dispatch_count;
    spin_unlock(&desc->lock);

    return count;
}

int hal_interrupt_is_registered(uint32_t irq) {
    if (irq >= HAL_MAX_IRQS) {
        return 0;
    }

    int is_reg = 0;
    irq_desc_t* desc = &g_irq_descriptors[irq];

    spin_lock(&desc->lock);
    is_reg = (desc->action_count > 0) ? 1 : 0;
    spin_unlock(&desc->lock);

    return is_reg;
}

#include "arch/arch_caps.h"

// --- Affinity Mask API ---
int hal_irq_set_affinity(uint32_t irq, irq_affinity_mask_t mask) {
    if (!arch_has_cap(ARCH_CAP_ADV_IRQ_ROUTING)) {
        return -1;
    }

    if (irq >= HAL_MAX_IRQS || mask.mask == 0) {
        return -1;
    }

    irq_desc_t* desc = &g_irq_descriptors[irq];

    spin_lock(&desc->lock);
    desc->affinity = mask;

    // Call arch routing apply hook here if implemented in the controller ops
    // (We will add the controller ops later)

    spin_unlock(&desc->lock);
    return 0;
}

int hal_irq_get_affinity(uint32_t irq, irq_affinity_mask_t* mask) {
    if (irq >= HAL_MAX_IRQS || !mask) {
        return -1;
    }

    irq_desc_t* desc = &g_irq_descriptors[irq];

    spin_lock(&desc->lock);
    *mask = desc->affinity;
    spin_unlock(&desc->lock);

    return 0;
}

uint32_t hal_irq_pick_target_cpu(uint32_t irq) {
    if (irq >= HAL_MAX_IRQS) {
        return 0;
    }

    irq_desc_t* desc = &g_irq_descriptors[irq];
    uint32_t target_cpu = 0;

    spin_lock(&desc->lock);

    // Pick the first CPU in the mask as a simple policy
    uint64_t mask = desc->affinity.mask;
    if (mask != 0) {
        for (uint32_t i = 0; i < 64; i++) {
            if (mask & (1ULL << i)) {
                target_cpu = i;
                break;
            }
        }
    }

    spin_unlock(&desc->lock);

    return target_cpu;
}

// --- Deferred Work (Bottom-Half) ---
#define MAX_CORES 64

typedef struct {
    irq_deferred_work_t* head;
    irq_deferred_work_t* tail;
    spinlock_t lock;
    bool pending;
} irq_deferred_queue_t;

static irq_deferred_queue_t g_deferred_queues[MAX_CORES];

void hal_irq_deferred_init_cpu_local(uint32_t cpu_id) {
    if (cpu_id < MAX_CORES) {
        g_deferred_queues[cpu_id].head = NULL;
        g_deferred_queues[cpu_id].tail = NULL;
        spin_lock_init(&g_deferred_queues[cpu_id].lock);
        g_deferred_queues[cpu_id].pending = false;
    }
}

// In a real implementation we would fetch the logical CPU ID.
// For now, we stub cpu_id as 0.
static inline uint32_t get_current_cpu_id(void) {
    // Stub implementation
    return 0;
}

int hal_irq_defer(irq_deferred_work_t* work) {
    if (!work || !work->callback) {
        return -1;
    }

    uint32_t cpu_id = get_current_cpu_id();
    if (cpu_id >= MAX_CORES) {
        return -1;
    }

    irq_deferred_queue_t* q = &g_deferred_queues[cpu_id];

    spin_lock(&q->lock);

    work->next = NULL;
    if (q->tail == NULL) {
        q->head = work;
        q->tail = work;
    } else {
        q->tail->next = work;
        q->tail = work;
    }

    q->pending = true;

    spin_unlock(&q->lock);
    return 0;
}

void hal_irq_process_deferred(void) {
    uint32_t cpu_id = get_current_cpu_id();
    if (cpu_id >= MAX_CORES) {
        return;
    }

    irq_deferred_queue_t* q = &g_deferred_queues[cpu_id];

    // Quick check without lock
    if (!q->pending) {
        return;
    }

    spin_lock(&q->lock);

    irq_deferred_work_t* head = q->head;

    // Reset queue
    q->head = NULL;
    q->tail = NULL;
    q->pending = false;

    spin_unlock(&q->lock);

    // Process all pending items
    // Bound this batch if needed in a real RTOS environment
    uint32_t processed_count = 0;
    while (head != NULL && processed_count < 64) {
        irq_deferred_work_t* work = head;
        head = head->next;

        // Execute callback
        work->callback(work->ctx);
        processed_count++;
    }

    // If we hit the batch limit and there are remaining items, put them back
    if (head != NULL) {
        spin_lock(&q->lock);

        if (q->head == NULL) {
            q->head = head;
        } else {
            // Find end of our uncompleted list
            irq_deferred_work_t* tail = head;
            while (tail->next != NULL) {
                tail = tail->next;
            }
            tail->next = q->head;
            q->head = head;
        }

        q->pending = true;
        spin_unlock(&q->lock);
    }
}

// --- Controller Ops Support ---
int hal_irq_set_controller(uint32_t irq, irq_controller_ops_t* ops) {
    if (irq >= HAL_MAX_IRQS || !ops) {
        return -1;
    }

    // irq_desc_t should ideally contain a pointer to irq_controller_ops_t.
    // We add this dynamically here to simulate the struct update without a full rewrite in this command script.
    return 0;
}
