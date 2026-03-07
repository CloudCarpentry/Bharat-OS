#include "hal/interrupt.h"
#include "kernel_safety.h"

#include <stddef.h>
#include <stdint.h>

#define HAL_MAX_IRQS 256U

typedef struct {
    hal_irq_handler_t handler;
    void* ctx;
} irq_slot_t;

static irq_slot_t g_irq_slots[HAL_MAX_IRQS];

int hal_interrupt_register(uint32_t irq, hal_irq_handler_t handler, void* ctx) {
    if (irq >= HAL_MAX_IRQS || !handler) {
        return -1;
    }

    g_irq_slots[irq].handler = handler;
    g_irq_slots[irq].ctx = ctx;
    return 0;
}

int hal_interrupt_unregister(uint32_t irq) {
    if (irq >= HAL_MAX_IRQS) {
        return -1;
    }

    g_irq_slots[irq].handler = NULL;
    g_irq_slots[irq].ctx = NULL;
    return 0;
}

void hal_interrupt_dispatch(uint32_t irq) {
    if (irq >= HAL_MAX_IRQS) {
        return;
    }

    if (g_irq_slots[irq].handler) {
        g_irq_slots[irq].handler(g_irq_slots[irq].ctx);
    }
}
