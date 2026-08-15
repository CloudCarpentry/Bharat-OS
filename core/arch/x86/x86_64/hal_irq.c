#include "hal/hal_irq.h"
#include "hal/hal_ipi.h"
#include "hal/hal_boot.h"
#include "device/irq_domain.h"
#include "irq/bh_irq.h"
#include "mm/physmap.h"

#define LAPIC_SVR_OFFSET 0x0F0
#define LAPIC_ICR_LOW_OFFSET 0x300
#define LAPIC_ICR_HIGH_OFFSET 0x310
#define LAPIC_EOI_OFFSET 0x0B0

uint32_t* g_lapic_base = (uint32_t*)0xFEE00000;

static inline void lapic_write(uint32_t offset, uint32_t value) {
    *(volatile uint32_t*)((uint64_t)g_lapic_base + offset) = value;
}

inline uint32_t lapic_read(uint32_t offset) {
    return *(volatile uint32_t*)((uint64_t)g_lapic_base + offset);
}

static inline void x86_outb(uint16_t port, uint8_t value) {
  __asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

static irq_domain_t* g_apic_root_domain = NULL;
static uint32_t g_virq_to_vector[256];

static inline uint32_t ioapic_read_reg(uintptr_t base, uint8_t offset) {
    volatile uint32_t *sel = (volatile uint32_t *)base;
    volatile uint32_t *win = (volatile uint32_t *)(base + 0x10);
    *sel = offset;
    return *win;
}

static inline void ioapic_write_reg(uintptr_t base, uint8_t offset, uint32_t val) {
    volatile uint32_t *sel = (volatile uint32_t *)base;
    volatile uint32_t *win = (volatile uint32_t *)(base + 0x10);
    *sel = offset;
    *win = val;
}

static irq_controller_desc_t *find_ioapic_for_gsi(uint32_t gsi, uint32_t *out_pin) {
    system_discovery_t* disc = hal_get_system_discovery();
    if (!disc) return NULL;

    for (uint32_t i = 0; i < disc->irq_ctrl_count; i++) {
        irq_controller_desc_t *ctrl = &disc->irq_ctrls[i];
        if (ctrl->type == IRQ_CTRL_IOAPIC) {
            uintptr_t vbase = (uintptr_t)physmap_phys_to_virt(ctrl->base);
            uint32_t ver = ioapic_read_reg(vbase, 0x01);
            uint32_t entries = ((ver >> 16) & 0xFF) + 1;
            if (gsi >= ctrl->gsi_base && gsi < ctrl->gsi_base + entries) {
                if (out_pin) *out_pin = gsi - ctrl->gsi_base;
                return ctrl;
            }
        }
    }
    return NULL;
}

static void ioapic_set_rte(uint32_t gsi, uint8_t vector, uint32_t target_cpu, bool level, bool active_low, bool masked) {
    uint32_t pin = 0;
    irq_controller_desc_t *ctrl = find_ioapic_for_gsi(gsi, &pin);
    if (!ctrl) return;

    uintptr_t vbase = (uintptr_t)physmap_phys_to_virt(ctrl->base);

    uint32_t low = vector;
    if (active_low) low |= (1U << 13);
    if (level) low |= (1U << 15);
    if (masked) low |= (1U << 16);

    uint32_t lapic_id = target_cpu;
    system_discovery_t *disc = hal_get_system_discovery();
    if (disc) {
        for (uint32_t i = 0; i < disc->topology.cpu_count; i++) {
            if (disc->topology.cpus[i].cpu_id == target_cpu) {
                lapic_id = disc->topology.cpus[i].hw_id;
                break;
            }
        }
    }

    uint32_t high = lapic_id << 24;

    uint8_t reg = 0x10 + (uint8_t)(pin * 2);
    ioapic_write_reg(vbase, reg, low);
    ioapic_write_reg(vbase, reg + 1, high);
}

void hal_irq_init_boot(void) {
    hal_irq_generic_init_boot();

    for (uint32_t i = 0; i < 256; i++) {
        g_virq_to_vector[i] = 0;
    }

    // Create root domain for APIC
    g_apic_root_domain = irq_domain_create("apic-root", 0, 256, NULL);
    if (g_apic_root_domain) {
        for (uint32_t i = 0; i < 256; i++) {
            irq_domain_map(g_apic_root_domain, i, i);
        }
        irq_domain_set_default(g_apic_root_domain);
    }

    // Enable local APIC (set Spurious Interrupt Vector Register)
    lapic_write(LAPIC_SVR_OFFSET, 0x1FF | 0x100);

    // Disable 8259 PICs to ensure only APIC is used
    x86_outb(0xA1, 0xFF);
    x86_outb(0x21, 0xFF);

    // Initialize IOAPICs and mask all entries by default
    system_discovery_t* disc = hal_get_system_discovery();
    if (disc) {
        for (uint32_t i = 0; i < disc->irq_ctrl_count; i++) {
            irq_controller_desc_t *ctrl = &disc->irq_ctrls[i];
            if (ctrl->type == IRQ_CTRL_IOAPIC) {
                uintptr_t vbase = (uintptr_t)physmap_phys_to_virt(ctrl->base);
                uint32_t ver = ioapic_read_reg(vbase, 0x01);
                uint32_t entries = ((ver >> 16) & 0xFF) + 1;
                for (uint32_t pin = 0; pin < entries; pin++) {
                    uint8_t reg = 0x10 + (uint8_t)(pin * 2);
                    ioapic_write_reg(vbase, reg, 1U << 16); // Masked
                    ioapic_write_reg(vbase, reg + 1, 0);
                }
            }
        }
    }
}

void hal_irq_init_cpu_local(uint32_t cpu_id) {
    (void)cpu_id;
    lapic_write(LAPIC_SVR_OFFSET, 0x1FF | 0x100);
}

int hal_irq_enable(uint32_t virq) {
    uint32_t pin = 0;
    irq_controller_desc_t *ctrl = find_ioapic_for_gsi(virq, &pin);
    if (ctrl) {
        uintptr_t vbase = (uintptr_t)physmap_phys_to_virt(ctrl->base);
        uint8_t reg = 0x10 + (uint8_t)(pin * 2);
        uint32_t low = ioapic_read_reg(vbase, reg);
        low &= ~(1U << 16); // Unmask
        ioapic_write_reg(vbase, reg, low);
        return 0;
    } else {
        // If it's not a GSI, allocate vector automatically
        if (g_virq_to_vector[virq] == 0) {
            bh_irq_vector_set_t vec_set;
            if (bh_irq_vector_alloc(1, 1, 0, &vec_set) == K_OK) {
                g_virq_to_vector[virq] = vec_set.start;
            }
        }
    }
    return 0;
}

int hal_irq_disable(uint32_t virq) {
    uint32_t pin = 0;
    irq_controller_desc_t *ctrl = find_ioapic_for_gsi(virq, &pin);
    if (ctrl) {
        uintptr_t vbase = (uintptr_t)physmap_phys_to_virt(ctrl->base);
        uint8_t reg = 0x10 + (uint8_t)(pin * 2);
        uint32_t low = ioapic_read_reg(vbase, reg);
        low |= (1U << 16); // Mask
        ioapic_write_reg(vbase, reg, low);
        return 0;
    }
    return 0;
}

uint32_t hal_irq_claim(void) {
    return 0;
}

void hal_irq_eoi(uint32_t irq) {
    (void)irq;
    lapic_write(LAPIC_EOI_OFFSET, 0);
}

void hal_ipi_init_cpu_local(uint32_t cpu_id) {
    (void)cpu_id;
}

void hal_ipi_send(uint32_t target_cpu, hal_ipi_reason_t reason) {
    uint32_t reason_vector = 200 + (uint32_t)reason;
    lapic_write(LAPIC_ICR_HIGH_OFFSET, (target_cpu << 24));
    lapic_write(LAPIC_ICR_LOW_OFFSET, reason_vector | 0x00004000);
}

void hal_ipi_broadcast(uint64_t mask, hal_ipi_reason_t reason) {
    for (uint32_t i = 0; i < 64; i++) {
        if ((mask & (1ULL << i)) != 0) {
            hal_ipi_send(i, reason);
        }
    }
}

// --- x86_64 MSI Support via Controller Ops ---
static int x86_64_compose_msi_message(uint32_t irq, uint64_t* msi_address, uint32_t* msi_data) {
    if (!msi_address || !msi_data) return -1;

    uint32_t target_cpu = hal_irq_pick_target_cpu(irq);
    uint32_t lapic_id = target_cpu;
    system_discovery_t *disc = hal_get_system_discovery();
    if (disc) {
        for (uint32_t i = 0; i < disc->topology.cpu_count; i++) {
            if (disc->topology.cpus[i].cpu_id == target_cpu) {
                lapic_id = disc->topology.cpus[i].hw_id;
                break;
            }
        }
    }

    *msi_address = 0xFEE00000ULL | ((uint64_t)lapic_id << 12);

    uint32_t vector = g_virq_to_vector[irq];
    if (vector == 0) {
        // Fallback to irq if no vector allocated
        vector = irq + 32;
    }

    *msi_data = (vector & 0xFF);
    return 0;
}

static void x86_64_irq_mask(uint32_t irq) {
    hal_irq_disable(irq);
}

static void x86_64_irq_unmask(uint32_t irq) {
    hal_irq_enable(irq);
}

static void x86_64_irq_ack(uint32_t irq) {
    (void)irq;
}

static void x86_64_irq_eoi_op(uint32_t irq) {
    hal_irq_eoi(irq);
}

static int x86_64_irq_set_affinity(uint32_t irq, irq_affinity_mask_t mask) {
    uint32_t target_cpu = 0;
    for (uint32_t i = 0; i < 64; i++) {
        if (mask.mask & (1ULL << i)) {
            target_cpu = i;
            break;
        }
    }

    uint32_t vector = g_virq_to_vector[irq];
    if (vector == 0) vector = irq + 32;

    uint32_t pin = 0;
    irq_controller_desc_t *ctrl = find_ioapic_for_gsi(irq, &pin);
    if (ctrl) {
        bool level = (irq >= 16);
        bool active_low = (irq >= 16);
        ioapic_set_rte(irq, (uint8_t)vector, target_cpu, level, active_low, false);
    }
    return 0;
}

irq_controller_ops_t g_x86_64_msi_ops = {
    .mask = x86_64_irq_mask,
    .unmask = x86_64_irq_unmask,
    .ack = x86_64_irq_ack,
    .eoi = x86_64_irq_eoi_op,
    .set_affinity = x86_64_irq_set_affinity,
    .compose_msi_message = x86_64_compose_msi_message
};

void hal_x86_64_init_msi_controller(uint32_t irq) {
    hal_irq_set_controller(irq, &g_x86_64_msi_ops);
}

void _secondary_trampoline(void) {
    // Dummy trampoline to resolve unconditional symbol reference in smp_boot.c on x86_64
}
