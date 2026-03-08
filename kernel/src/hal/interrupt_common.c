#include "hal/interrupt.h"
#include "kernel_safety.h"

#include <stddef.h>
#include <stdint.h>

#define HAL_MAX_IRQS 256U

typedef struct {
    hal_irq_handler_t handler;
    void* ctx;
    uint64_t dispatch_count;
} irq_slot_t;

static irq_slot_t g_irq_slots[HAL_MAX_IRQS];

int hal_interrupt_register(uint32_t irq, hal_irq_handler_t handler, void* ctx) {
    if (irq >= HAL_MAX_IRQS || !handler) {
        return -1;
    }

    g_irq_slots[irq].handler = handler;
    g_irq_slots[irq].ctx = ctx;
    g_irq_slots[irq].dispatch_count = 0U;
    return 0;
}

int hal_interrupt_unregister(uint32_t irq) {
    if (irq >= HAL_MAX_IRQS) {
        return -1;
    }

    g_irq_slots[irq].handler = NULL;
    g_irq_slots[irq].ctx = NULL;
    g_irq_slots[irq].dispatch_count = 0U;
    return 0;
}

void hal_interrupt_dispatch(uint32_t irq) {
    if (irq >= HAL_MAX_IRQS) {
        return;
    }

    if (g_irq_slots[irq].handler) {
        g_irq_slots[irq].dispatch_count++;
        g_irq_slots[irq].handler(g_irq_slots[irq].ctx);
    }
}

uint64_t hal_interrupt_get_dispatch_count(uint32_t irq) {
    if (irq >= HAL_MAX_IRQS) {
        return 0U;
    }
    return g_irq_slots[irq].dispatch_count;
}

int hal_interrupt_is_registered(uint32_t irq) {
    if (irq >= HAL_MAX_IRQS) {
        return 0;
    }
    return (g_irq_slots[irq].handler != NULL) ? 1 : 0;
}
