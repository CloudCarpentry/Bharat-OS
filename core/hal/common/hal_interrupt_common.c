#include "hal/hal_irq.h"
#include "irq/bh_irq.h"
#include "device/irq_domain.h"
#include "kernel_safety.h"
#include "spinlock.h"

static irq_controller_ops_t* g_hal_ops[256];
static bh_irq_controller_ops_t g_adapted_ops[256];

static void adapt_mask(uint32_t irq) {
    if (irq < 256 && g_hal_ops[irq] && g_hal_ops[irq]->mask) {
        g_hal_ops[irq]->mask(irq);
    }
}

static void adapt_unmask(uint32_t irq) {
    if (irq < 256 && g_hal_ops[irq] && g_hal_ops[irq]->unmask) {
        g_hal_ops[irq]->unmask(irq);
    }
}

static void adapt_ack(uint32_t irq) {
    if (irq < 256 && g_hal_ops[irq] && g_hal_ops[irq]->ack) {
        g_hal_ops[irq]->ack(irq);
    }
}

static void adapt_eoi(uint32_t irq) {
    if (irq < 256 && g_hal_ops[irq] && g_hal_ops[irq]->eoi) {
        g_hal_ops[irq]->eoi(irq);
    }
}

static int adapt_set_affinity(uint32_t irq, bh_irq_affinity_t affinity) {
    if (irq < 256 && g_hal_ops[irq] && g_hal_ops[irq]->set_affinity) {
        irq_affinity_mask_t mask = { .mask = affinity.mask };
        return g_hal_ops[irq]->set_affinity(irq, mask);
    }
    return -1;
}

static int adapt_compose_msi(uint32_t irq, uint64_t* address, uint32_t* data) {
    if (irq < 256 && g_hal_ops[irq] && g_hal_ops[irq]->compose_msi_message) {
        return g_hal_ops[irq]->compose_msi_message(irq, address, data);
    }
    return -1;
}

void hal_irq_generic_init_boot(void) {
    bh_irq_init_boot();
    for (uint32_t i = 0; i < 256; i++) {
        g_hal_ops[i] = NULL;
    }
}

int hal_interrupt_register(uint32_t irq, hal_irq_handler_t handler, void* ctx, uint32_t flags, const char* name, void* dev_id) {
    bh_irq_handle_t h;
    kstatus_t status = bh_irq_register(irq, (bh_irq_handler_t)handler, ctx, flags, name, dev_id, &h);
    return (status == K_OK) ? 0 : -1;
}

int hal_interrupt_unregister(uint32_t irq, void* dev_id) {
    bh_irq_desc_t* desc = bh_irq_get_descriptor(irq);
    if (!desc) return -1;
    bh_irq_handle_t h = bh_irq_make_handle(irq, desc->generation, 0, 1);
    kstatus_t status = bh_irq_unregister(h, dev_id);
    return (status == K_OK) ? 0 : -1;
}

void hal_interrupt_dispatch(uint32_t irq) {
    bh_irq_dispatch(irq);
}

uint64_t hal_interrupt_get_dispatch_count(uint32_t irq) {
    bh_irq_desc_t* desc = bh_irq_get_descriptor(irq);
    return desc ? desc->dispatch_count : 0;
}

int hal_interrupt_is_registered(uint32_t irq) {
    return bh_irq_is_registered(irq) ? 1 : 0;
}

int hal_irq_set_affinity(uint32_t irq, irq_affinity_mask_t mask) {
    bh_irq_desc_t* desc = bh_irq_get_descriptor(irq);
    if (!desc) return -1;
    bh_irq_handle_t h = bh_irq_make_handle(irq, desc->generation, 0, 1);
    bh_irq_affinity_t aff = { .mask = mask.mask };
    kstatus_t status = bh_irq_set_affinity(h, aff);
    return (status == K_OK) ? 0 : -1;
}

int hal_irq_get_affinity(uint32_t irq, irq_affinity_mask_t* mask) {
    if (!mask) return -1;
    bh_irq_desc_t* desc = bh_irq_get_descriptor(irq);
    if (!desc) return -1;
    bh_irq_handle_t h = bh_irq_make_handle(irq, desc->generation, 0, 1);
    bh_irq_affinity_t aff;
    kstatus_t status = bh_irq_get_affinity(h, &aff);
    if (status == K_OK) {
        mask->mask = aff.mask;
        return 0;
    }
    return -1;
}

uint32_t hal_irq_pick_target_cpu(uint32_t irq) {
    return bh_irq_pick_target_cpu(irq);
}

int hal_irq_set_controller(uint32_t irq, irq_controller_ops_t* ops) {
    if (irq >= 256 || !ops) return -1;
    g_hal_ops[irq] = ops;
    g_adapted_ops[irq].mask = adapt_mask;
    g_adapted_ops[irq].unmask = adapt_unmask;
    g_adapted_ops[irq].ack = adapt_ack;
    g_adapted_ops[irq].eoi = adapt_eoi;
    g_adapted_ops[irq].set_affinity = adapt_set_affinity;
    g_adapted_ops[irq].compose_msi_message = adapt_compose_msi;

    kstatus_t status = bh_irq_set_controller(irq, &g_adapted_ops[irq]);
    return (status == K_OK) ? 0 : -1;
}

void hal_interrupt_handle_trap_irq(uint64_t hw_cause,
                                   void (*timer_handler)(void),
                                   hal_irq_dispatch_fn_t dispatch_fn,
                                   void* dispatch_ctx) {
    uint32_t hwirq = hal_interrupt_get_active_irq(hw_cause);

    // Timer vector is often handled outside generic routing, check raw hwirq first
    if (hwirq == hal_irq_timer_vector() && timer_handler) {
        timer_handler();
        hal_irq_eoi(hwirq);
        return;
    }

    uint32_t virq = 0;
    irq_domain_t* root_domain = irq_domain_get_default();

    if (!root_domain || irq_domain_translate(root_domain, hwirq, &virq) != 0) {
        hal_irq_eoi(hwirq);
        return;
    }

    if (dispatch_fn) {
        dispatch_fn(virq, dispatch_ctx);
    } else {
        bh_irq_dispatch(virq);
    }
}

// --- Forwarding Bottom-Half APIs ---
int hal_irq_defer(irq_deferred_work_t* work) {
    if (!work || !work->callback) return -1;
    bh_irq_handle_t handle = bh_irq_make_handle(work->source_irq, 1, 0, 1);
    kstatus_t status = bh_irq_defer_submit((bh_irq_deferred_cb_t)work->callback, work->ctx, handle);
    return (status == K_OK) ? 0 : -1;
}

void hal_irq_process_deferred(void) {
    bh_irq_process_deferred();
}

void hal_irq_deferred_init_cpu_local(uint32_t cpu_id) {
    bh_irq_deferred_init_cpu_local(cpu_id);
}
