#include "hal/hal_irq.h"
#include "device/irq_domain.h"

// --- PLIC Definitions (QEMU virt, same base as riscv64) ---
#define PLIC_BASE           0x0c000000UL
#define PLIC_PRIORITY       PLIC_BASE
#define PLIC_ENABLE         (PLIC_BASE + 0x2000)
#define PLIC_THRESHOLD      (PLIC_BASE + 0x200000)
#define PLIC_CLAIM          (PLIC_BASE + 0x200004)

#define PLIC_ENABLE_CTX(ctx)    (PLIC_ENABLE    + (ctx) * 0x80)
#define PLIC_THRESHOLD_CTX(ctx) (PLIC_THRESHOLD + (ctx) * 0x1000)
#define PLIC_CLAIM_CTX(ctx)     (PLIC_CLAIM     + (ctx) * 0x1000)

static irq_domain_t* g_plic_root_domain = NULL;

void hal_irq_init_boot(void) {
    hal_irq_generic_init_boot();

    g_plic_root_domain = irq_domain_create("plic-root", 0, 64, NULL);
    if (g_plic_root_domain) {
        for (uint32_t i = 0; i < 64; i++) {
            irq_domain_map(g_plic_root_domain, i, i);
        }
        irq_domain_set_default(g_plic_root_domain);
    }
}

void hal_irq_init_cpu_local(uint32_t cpu_id) {
    uint32_t ctx = cpu_id * 2 + 1; // Hart * 2 + 1 for S-mode
    volatile uint32_t *threshold = (volatile uint32_t *)PLIC_THRESHOLD_CTX(ctx);
    *threshold = 0;
}

int hal_irq_enable(uint32_t vector) {
    (void)vector;
    return 0;
}

int hal_irq_disable(uint32_t vector) {
    (void)vector;
    return 0;
}

uint32_t hal_irq_claim(void) {
    uint32_t ctx = 1; // S-mode context for hart 0
    volatile uint32_t *claim_reg = (volatile uint32_t *)PLIC_CLAIM_CTX(ctx);
    return *claim_reg;
}

void hal_irq_eoi(uint32_t irq) {
    uint32_t ctx = 1;
    volatile uint32_t *claim_reg = (volatile uint32_t *)PLIC_CLAIM_CTX(ctx);
    *claim_reg = irq;
}

// Required by hal_interrupt_common.c
void hal_interrupt_end_of_interrupt(uint32_t irq) {
    hal_irq_eoi(irq);
}

int hal_interrupt_controller_init(void) {
    return 0;
}

uint32_t hal_interrupt_acknowledge(void) {
    return hal_irq_claim();
}

int hal_interrupt_route(uint32_t irq, uint32_t target_core) {
    if (irq == 0 || irq > 53) return -1;
    volatile uint32_t *priority = (volatile uint32_t *)(PLIC_PRIORITY + irq * 4);
    *priority = 1;
    uint32_t ctx = target_core * 2 + 1;
    volatile uint32_t *enable = (volatile uint32_t *)PLIC_ENABLE_CTX(ctx);
    *enable |= (1UL << (irq % 32));
    return 0;
}
