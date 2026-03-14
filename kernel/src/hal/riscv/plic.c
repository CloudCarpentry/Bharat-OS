#include "hal/hal_irq.h"

// --- PLIC Definitions (QEMU virt) ---
#define PLIC_BASE 0x0c000000ULL
#define PLIC_PRIORITY PLIC_BASE
#define PLIC_PENDING (PLIC_BASE + 0x1000)
#define PLIC_ENABLE (PLIC_BASE + 0x2000)
#define PLIC_THRESHOLD (PLIC_BASE + 0x200000)
#define PLIC_CLAIM (PLIC_BASE + 0x200004)

// Hart context calculation (assuming Hart 0 Supervisor context is context 1)
#define PLIC_ENABLE_CTX(ctx) (PLIC_ENABLE + (ctx) * 0x80)
#define PLIC_THRESHOLD_CTX(ctx) (PLIC_THRESHOLD + (ctx) * 0x1000)
#define PLIC_CLAIM_CTX(ctx) (PLIC_CLAIM + (ctx) * 0x1000)

void hal_irq_init_boot(void) {
    // Basic PLIC initialization on boot
}

void hal_irq_init_cpu_local(uint32_t cpu_id) {
    // For simplicity, we initialize PLIC for Hart (cpu_id), Supervisor Mode
    uint32_t ctx = cpu_id * 2 + 1; // Hart * 2 + 1 assuming S-mode

    // Set context threshold to 0 (accept all interrupts)
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
    // Needs logical core ID, defaulting to 0 / S-mode context
    uint32_t ctx = 1;
    volatile uint32_t *claim_reg = (volatile uint32_t *)PLIC_CLAIM_CTX(ctx);
    return *claim_reg;
}

void hal_irq_eoi(uint32_t irq) {
    uint32_t ctx = 1; // S-mode context
    volatile uint32_t *claim_reg = (volatile uint32_t *)PLIC_CLAIM_CTX(ctx);
    *claim_reg = irq;
}
