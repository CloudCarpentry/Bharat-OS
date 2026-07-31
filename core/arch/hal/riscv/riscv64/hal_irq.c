#include "hal/hal_irq.h"
#include "hal/hal.h"
#include "device/irq_domain.h"

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

static irq_domain_t* g_plic_root_domain = NULL;

void hal_irq_init_boot(void) {
    hal_irq_generic_init_boot();

    // Create root domain for PLIC
    g_plic_root_domain = irq_domain_create("plic-root", 0, 64, NULL);
    if (g_plic_root_domain) {
        for (uint32_t i = 0; i < 64; i++) {
            irq_domain_map(g_plic_root_domain, i, i);
        }
        irq_domain_set_default(g_plic_root_domain);
    }

    hal_serial_write("BHARAT_IRQ:ROOT_DOMAIN=READY arch=riscv64\n");
}

void hal_irq_init_cpu_local(uint32_t cpu_id) {
    uint32_t ctx = cpu_id * 2 + 1; // S-mode context

    // Set context threshold to 0 (accept all interrupts)
    volatile uint32_t *threshold = (volatile uint32_t *)PLIC_THRESHOLD_CTX(ctx);
    *threshold = 0;
}

int hal_irq_enable(uint32_t vector) {
    if (vector == 0 || vector >= 64) return -1;
    uint32_t cpu_id = hal_cpu_get_id();
    uint32_t ctx = cpu_id * 2 + 1; // S-mode context

    // Set priority of the vector to 1 (or default)
    volatile uint32_t *priority_reg = (volatile uint32_t *)(PLIC_PRIORITY + vector * 4);
    *priority_reg = 1;

    volatile uint32_t *enable_reg = (volatile uint32_t *)(PLIC_ENABLE_CTX(ctx) + (vector / 32) * 4);
    *enable_reg |= (1U << (vector % 32));
    return 0;
}

int hal_irq_disable(uint32_t vector) {
    if (vector == 0 || vector >= 64) return -1;
    uint32_t cpu_id = hal_cpu_get_id();
    uint32_t ctx = cpu_id * 2 + 1;

    volatile uint32_t *enable_reg = (volatile uint32_t *)(PLIC_ENABLE_CTX(ctx) + (vector / 32) * 4);
    *enable_reg &= ~(1U << (vector % 32));
    return 0;
}

uint32_t hal_irq_claim(void) {
    uint32_t cpu_id = hal_cpu_get_id();
    uint32_t ctx = cpu_id * 2 + 1;
    volatile uint32_t *claim_reg = (volatile uint32_t *)PLIC_CLAIM_CTX(ctx);
    return *claim_reg;
}

void hal_irq_eoi(uint32_t irq) {
    uint32_t cpu_id = hal_cpu_get_id();
    uint32_t ctx = cpu_id * 2 + 1;
    volatile uint32_t *claim_reg = (volatile uint32_t *)PLIC_CLAIM_CTX(ctx);
    *claim_reg = irq;
}
