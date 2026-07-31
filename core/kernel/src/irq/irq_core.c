#include "irq/bh_irq.h"
#include "hal/hal.h"
#include "kernel_safety.h"
#include "spinlock.h"
#include "device/irq_domain.h"

#define BH_IRQ_MAX_DESCRIPTORS 256U

// --- GCC Atomics Helpers ---
#define atomic_load(ptr) __atomic_load_n(ptr, __ATOMIC_SEQ_CST)
#define atomic_store(ptr, val) __atomic_store_n(ptr, val, __ATOMIC_SEQ_CST)
#define atomic_fetch_add(ptr, val) __atomic_fetch_add(ptr, val, __ATOMIC_SEQ_CST)
#define atomic_fetch_sub(ptr, val) __atomic_fetch_sub(ptr, val, __ATOMIC_SEQ_CST)

static bh_irq_desc_t g_irq_descriptors[BH_IRQ_MAX_DESCRIPTORS];
static uint32_t g_irq_nesting[MAX_CPUS];

void bh_irq_init_boot(void) {
    for (uint32_t i = 0; i < BH_IRQ_MAX_DESCRIPTORS; i++) {
        bh_irq_desc_t* desc = &g_irq_descriptors[i];
        desc->virq = i;
        desc->generation = 1;
        atomic_store(&desc->state, BH_IRQ_STATE_FREE);
        atomic_store(&desc->in_flight, 0);
        desc->flags = 0;
        desc->trigger_type = BH_IRQ_TRIGGER_NONE;
        desc->affinity.mask = ~0ULL;
        desc->domain = NULL;
        desc->controller = NULL;
        desc->dispatch_count = 0;
        desc->handled_count = 0;
        desc->spurious_count = 0;
        desc->deferred_count = 0;
        desc->dropped_deferred_count = 0;

        spin_lock_init(&desc->lock);
        sched_wait_queue_init(&desc->quiesce_wait);

        for (uint32_t j = 0; j < BH_IRQ_MAX_SHARED_ACTIONS; j++) {
            desc->actions[j].handler = NULL;
            desc->actions[j].ctx = NULL;
            desc->actions[j].dev_id = NULL;
            desc->actions[j].name = NULL;
            desc->actions[j].flags = 0;
            desc->actions[j].dispatch_count = 0;
            atomic_store(&desc->actions[j].active_refs, 0);
            atomic_store(&desc->actions[j].state, BH_IRQ_ACTION_STATE_FREE);
        }
    }

    for (uint32_t i = 0; i < MAX_CPUS; i++) {
        atomic_store(&g_irq_nesting[i], 0);
    }
}

void bh_irq_init_cpu_local(uint32_t cpu_id) {
    if (cpu_id < MAX_CPUS) {
        atomic_store(&g_irq_nesting[cpu_id], 0);
    }
}

bool bh_irq_in_hardirq(void) {
    uint32_t cpu_id = hal_cpu_get_id();
    if (cpu_id < MAX_CPUS) {
        return atomic_load(&g_irq_nesting[cpu_id]) > 0;
    }
    return false;
}

kstatus_t bh_irq_register(uint32_t virq, bh_irq_handler_t handler, void* ctx, uint32_t flags, const char* name, void* dev_id, bh_irq_handle_t* out_handle) {
    if (virq >= BH_IRQ_MAX_DESCRIPTORS || !handler || !dev_id) {
        return K_ERR_INVALID_ARG;
    }

    bh_irq_desc_t* desc = &g_irq_descriptors[virq];
    spin_lock(&desc->lock);

    uint32_t curr_state = atomic_load(&desc->state);
    if (curr_state == BH_IRQ_STATE_FREE) {
        desc->action_count = 0;
        desc->flags = 0;
        atomic_store(&desc->state, BH_IRQ_STATE_ALLOCATED);
    }

    if (desc->action_count > 0) {
        if (!(desc->flags & BH_IRQF_SHARED) || !(flags & BH_IRQF_SHARED)) {
            spin_unlock(&desc->lock);
            return K_ERR_DENIED;
        }
    }

    for (uint32_t i = 0; i < BH_IRQ_MAX_SHARED_ACTIONS; i++) {
        if (atomic_load(&desc->actions[i].state) != BH_IRQ_ACTION_STATE_FREE && desc->actions[i].dev_id == dev_id) {
            spin_unlock(&desc->lock);
            return K_ERR_DENIED;
        }
    }

    int slot = -1;
    for (uint32_t i = 0; i < BH_IRQ_MAX_SHARED_ACTIONS; i++) {
        if (atomic_load(&desc->actions[i].state) == BH_IRQ_ACTION_STATE_FREE) {
            slot = (int)i;
            break;
        }
    }

    if (slot == -1) {
        spin_unlock(&desc->lock);
        return K_ERR_AGAIN;
    }

    desc->actions[slot].handler = handler;
    desc->actions[slot].ctx = ctx;
    desc->actions[slot].dev_id = dev_id;
    desc->actions[slot].name = name;
    desc->actions[slot].flags = flags;
    desc->actions[slot].dispatch_count = 0;
    atomic_store(&desc->actions[slot].active_refs, 0);
    atomic_store(&desc->actions[slot].state, BH_IRQ_ACTION_STATE_ACTIVE);

    desc->action_count++;
    if (desc->action_count == 1) {
        desc->flags = flags;
    } else {
        desc->flags |= flags;
    }

    atomic_store(&desc->state, BH_IRQ_STATE_ACTIVE);

    if (out_handle) {
        *out_handle = bh_irq_make_handle(virq, desc->generation, 0, 1);
    }

    spin_unlock(&desc->lock);
    return K_OK;
}

kstatus_t bh_irq_unregister(bh_irq_handle_t handle, void* dev_id) {
    uint16_t slot = bh_irq_handle_slot(handle);
    if (slot >= BH_IRQ_MAX_DESCRIPTORS || !dev_id) {
        return K_ERR_INVALID_ARG;
    }

    bh_irq_desc_t* desc = &g_irq_descriptors[slot];
    spin_lock(&desc->lock);

    if (desc->generation != bh_irq_handle_generation(handle)) {
        spin_unlock(&desc->lock);
        return K_ERR_BAD_STATE;
    }

    int act_idx = -1;
    for (uint32_t i = 0; i < BH_IRQ_MAX_SHARED_ACTIONS; i++) {
        if (atomic_load(&desc->actions[i].state) != BH_IRQ_ACTION_STATE_FREE && desc->actions[i].dev_id == dev_id) {
            act_idx = (int)i;
            break;
        }
    }

    if (act_idx == -1) {
        spin_unlock(&desc->lock);
        return K_ERR_NOT_FOUND;
    }

    atomic_store(&desc->actions[act_idx].state, BH_IRQ_ACTION_STATE_REMOVING);

    if (bh_irq_in_hardirq()) {
        bool self_unreg = false;
        if (atomic_load(&desc->actions[act_idx].active_refs) > 0) {
            self_unreg = true;
        }

        if (self_unreg) {
            spin_unlock(&desc->lock);
            return K_OK;
        } else {
            atomic_store(&desc->actions[act_idx].state, BH_IRQ_ACTION_STATE_ACTIVE);
            spin_unlock(&desc->lock);
            return K_ERR_BAD_STATE;
        }
    }

    spin_unlock(&desc->lock);

    while (atomic_load(&desc->actions[act_idx].active_refs) > 0) {
        bh_thread_yield();
    }

    spin_lock(&desc->lock);
    desc->actions[act_idx].state = BH_IRQ_ACTION_STATE_FREE;
    desc->actions[act_idx].handler = NULL;
    desc->actions[act_idx].ctx = NULL;
    desc->actions[act_idx].dev_id = NULL;
    desc->actions[act_idx].name = NULL;
    desc->actions[act_idx].flags = 0;
    desc->actions[act_idx].dispatch_count = 0;

    if (desc->action_count > 0) {
        desc->action_count--;
    }
    if (desc->action_count == 0) {
        atomic_store(&desc->state, BH_IRQ_STATE_FREE);
        desc->generation++;
    }

    spin_unlock(&desc->lock);
    return K_OK;
}

kstatus_t bh_irq_dispatch(uint32_t virq) {
    if (virq >= BH_IRQ_MAX_DESCRIPTORS) {
        return K_ERR_INVALID_ARG;
    }

    bh_irq_desc_t* desc = &g_irq_descriptors[virq];

    uint32_t cpu_id = hal_cpu_get_id();
    if (cpu_id < MAX_CPUS) {
        atomic_fetch_add(&g_irq_nesting[cpu_id], 1);
    }

    spin_lock(&desc->lock);

    uint32_t curr_state = atomic_load(&desc->state);
    if (curr_state == BH_IRQ_STATE_FREE || curr_state == BH_IRQ_STATE_DEAD) {
        spin_unlock(&desc->lock);
        if (cpu_id < MAX_CPUS) {
            atomic_fetch_sub(&g_irq_nesting[cpu_id], 1);
        }
        return K_ERR_NOT_FOUND;
    }

    atomic_fetch_add(&desc->in_flight, 1);
    desc->dispatch_count++;

    bh_irq_action_t active_actions[BH_IRQ_MAX_SHARED_ACTIONS];
    uint32_t active_count = 0;

    for (uint32_t i = 0; i < BH_IRQ_MAX_SHARED_ACTIONS; i++) {
        if (atomic_load(&desc->actions[i].state) == BH_IRQ_ACTION_STATE_ACTIVE) {
            atomic_fetch_add(&desc->actions[i].active_refs, 1);
            active_actions[active_count] = desc->actions[i];
            active_count++;
        }
    }

    spin_unlock(&desc->lock);

    if (desc->controller && desc->controller->ack) {
        desc->controller->ack(virq);
    }

    bool handled = false;
    for (uint32_t i = 0; i < active_count; i++) {
        bh_irq_return_t ret = active_actions[i].handler(active_actions[i].ctx);
        if (ret == BH_IRQ_HANDLED || ret == BH_IRQ_WAKE_DEFERRED) {
            handled = true;
        }
    }

    spin_lock(&desc->lock);

    for (uint32_t i = 0; i < BH_IRQ_MAX_SHARED_ACTIONS; i++) {
        for (uint32_t j = 0; j < active_count; j++) {
            if (desc->actions[i].dev_id == active_actions[j].dev_id && desc->actions[i].handler == active_actions[j].handler) {
                atomic_fetch_sub(&desc->actions[i].active_refs, 1);
                if (atomic_load(&desc->actions[i].state) == BH_IRQ_ACTION_STATE_REMOVING && atomic_load(&desc->actions[i].active_refs) == 0) {
                    desc->actions[i].state = BH_IRQ_ACTION_STATE_FREE;
                    desc->actions[i].handler = NULL;
                    desc->actions[i].ctx = NULL;
                    desc->actions[i].dev_id = NULL;
                    desc->actions[i].name = NULL;
                    desc->actions[i].flags = 0;
                    desc->actions[i].dispatch_count = 0;
                    if (desc->action_count > 0) {
                        desc->action_count--;
                    }
                    if (desc->action_count == 0) {
                        atomic_store(&desc->state, BH_IRQ_STATE_FREE);
                        desc->generation++;
                    }
                }
            }
        }
    }

    if (handled) {
        desc->handled_count++;
    } else {
        desc->spurious_count++;
    }

    atomic_fetch_sub(&desc->in_flight, 1);

    if (desc->controller && desc->controller->eoi) {
        desc->controller->eoi(virq);
    }

    spin_unlock(&desc->lock);

    if (cpu_id < MAX_CPUS) {
        atomic_fetch_sub(&g_irq_nesting[cpu_id], 1);
    }

    return handled ? K_OK : K_ERR_NOT_FOUND;
}

bool bh_irq_is_registered(uint32_t virq) {
    if (virq >= BH_IRQ_MAX_DESCRIPTORS) {
        return false;
    }
    bh_irq_desc_t* desc = &g_irq_descriptors[virq];
    spin_lock(&desc->lock);
    bool is_reg = (desc->action_count > 0);
    spin_unlock(&desc->lock);
    return is_reg;
}

kstatus_t bh_irq_synchronize(bh_irq_handle_t handle, uint64_t deadline_ticks) {
    uint16_t slot = bh_irq_handle_slot(handle);
    if (slot >= BH_IRQ_MAX_DESCRIPTORS) {
        return K_ERR_INVALID_ARG;
    }

    bh_irq_desc_t* desc = &g_irq_descriptors[slot];
    if (desc->generation != bh_irq_handle_generation(handle)) {
        return K_ERR_BAD_STATE;
    }

    uint64_t start_tick = sched_get_ticks();
    while (atomic_load(&desc->in_flight) > 0) {
        if (sched_get_ticks() - start_tick > deadline_ticks) {
            return K_ERR_TIMEOUT;
        }
        bh_thread_yield();
    }

    return K_OK;
}

kstatus_t bh_irq_set_affinity(bh_irq_handle_t handle, bh_irq_affinity_t affinity) {
    uint16_t slot = bh_irq_handle_slot(handle);
    if (slot >= BH_IRQ_MAX_DESCRIPTORS) {
        return K_ERR_INVALID_ARG;
    }

    bh_irq_desc_t* desc = &g_irq_descriptors[slot];
    spin_lock(&desc->lock);
    if (desc->generation != bh_irq_handle_generation(handle)) {
        spin_unlock(&desc->lock);
        return K_ERR_BAD_STATE;
    }

    desc->affinity = affinity;
    const bh_irq_controller_ops_t* controller = desc->controller;
    spin_unlock(&desc->lock);

    if (controller && controller->set_affinity) {
        if (controller->set_affinity(slot, affinity) != 0) {
            return K_ERR_UNSUPPORTED;
        }
    }

    return K_OK;
}

kstatus_t bh_irq_get_affinity(bh_irq_handle_t handle, bh_irq_affinity_t* out_affinity) {
    uint16_t slot = bh_irq_handle_slot(handle);
    if (slot >= BH_IRQ_MAX_DESCRIPTORS || !out_affinity) {
        return K_ERR_INVALID_ARG;
    }

    bh_irq_desc_t* desc = &g_irq_descriptors[slot];
    spin_lock(&desc->lock);
    if (desc->generation != bh_irq_handle_generation(handle)) {
        spin_unlock(&desc->lock);
        return K_ERR_BAD_STATE;
    }

    *out_affinity = desc->affinity;
    spin_unlock(&desc->lock);
    return K_OK;
}

uint32_t bh_irq_pick_target_cpu(uint32_t virq) {
    if (virq >= BH_IRQ_MAX_DESCRIPTORS) {
        return 0;
    }

    bh_irq_desc_t* desc = &g_irq_descriptors[virq];
    uint32_t target_cpu = 0;

    spin_lock(&desc->lock);
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

kstatus_t bh_irq_set_controller(uint32_t virq, const bh_irq_controller_ops_t* ops) {
    if (virq >= BH_IRQ_MAX_DESCRIPTORS || !ops) {
        return K_ERR_INVALID_ARG;
    }

    bh_irq_desc_t* desc = &g_irq_descriptors[virq];
    spin_lock(&desc->lock);
    desc->controller = ops;
    spin_unlock(&desc->lock);
    return K_OK;
}

bh_irq_desc_t* bh_irq_get_descriptor(uint32_t virq) {
    if (virq >= BH_IRQ_MAX_DESCRIPTORS) {
        return NULL;
    }
    return &g_irq_descriptors[virq];
}
