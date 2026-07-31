#include <stddef.h>
#include "hal/hal_irq.h"
#include "hal/hal.h"
#include "hal/hal_boot.h"
#include "hal/hal_discovery.h"
#include "device/irq_domain.h"

// Define registers for basic GICv3 operations
#define GICD_CTLR        0x0000
#define GICD_IGROUPR0    0x0080
#define GICR_WAKER       0x0014
#define GICR_IGROUPR0    0x0080

// ICC System Registers mapped via generic sysreg macros
#define read_icc_iar1_el1(val)  __asm__ volatile("mrs %0, S3_0_C12_C12_0" : "=r"(val))
#define write_icc_eoir1_el1(val) __asm__ volatile("msr S3_0_C12_C12_1, %0" : : "r"(val))
#define write_icc_sgi1r_el1(val) __asm__ volatile("msr S3_0_C12_C11_5, %0" : : "r"(val))

static void* g_gicd_base = NULL;
static void* g_gicr_base = NULL;
static void* g_its_base = NULL;

static irq_domain_t* g_gicv3_root_domain = NULL;

static inline void gicd_write(uint32_t offset, uint32_t value) {
    *(volatile uint32_t*)((uint64_t)g_gicd_base + offset) = value;
}

static inline void gicr_write(uint32_t offset, uint32_t value) {
    *(volatile uint32_t*)((uint64_t)g_gicr_base + offset) = value;
}

static inline uint32_t gicr_read(uint32_t offset) {
    return *(volatile uint32_t*)((uint64_t)g_gicr_base + offset);
}

static int its_alloc_msi(msi_domain_t* domain, void* device, int count, msi_desc_t* desc_array) {
    (void)domain;
    (void)device;
    (void)count;
    (void)desc_array;
    // GIC ITS is genuinely not supported in this phase - return K_ERR_UNSUPPORTED
    return -5;
}

static void its_free_msi(msi_domain_t* domain, msi_desc_t* desc_array, int count) {
    (void)domain;
    (void)desc_array;
    (void)count;
}

static msi_domain_t its_msi_domain = {
    .name = "gicv3-its",
    .base_domain = NULL,
    .alloc_msi = its_alloc_msi,
    .free_msi = its_free_msi,
    .write_msg = NULL,
    .host_data = NULL
};

void hal_irq_init_boot(void) {
    hal_irq_generic_init_boot();

    // Create root domain for GICv3
    g_gicv3_root_domain = irq_domain_create("gicv3-root", 0, 1024, NULL);
    if (g_gicv3_root_domain) {
        for (uint32_t i = 0; i < 256; i++) { // Bounded by HAL_MAX_IRQS (256)
            irq_domain_map(g_gicv3_root_domain, i, i);
        }
        irq_domain_set_default(g_gicv3_root_domain);
    }

    system_discovery_t* disc = hal_get_system_discovery();
    if (disc) {
        for (uint32_t i = 0; i < disc->irq_ctrl_count; i++) {
            if (disc->irq_ctrls[i].type == IRQ_CTRL_GICV3) {
                g_gicd_base = (void*)(uintptr_t)disc->irq_ctrls[i].base;
                g_gicr_base = (void*)(uintptr_t)disc->irq_ctrls[i].aux_base;
            } else if (disc->irq_ctrls[i].type == IRQ_CTRL_GIC_ITS) {
                g_its_base = (void*)(uintptr_t)disc->irq_ctrls[i].base;
                // Emit ITS unsupported markers & fallbacks to console
                hal_serial_write("BHARAT_IRQ:ITS=UNSUPPORTED\n");
                hal_serial_write("BHARAT_IRQ:MSI_FALLBACK=SPI\n");
            }
        }
    }

    if (!g_gicd_base) g_gicd_base = (void*)0x08000000;
    if (!g_gicr_base) g_gicr_base = (void*)0x080A0000;

    // Disable Distributor before config
    gicd_write(GICD_CTLR, 0);
    // Route interrupts to non-secure Group 1
    gicd_write(GICD_IGROUPR0, 0xFFFFFFFF);
    // Enable Group 1 interrupts
    gicd_write(GICD_CTLR, 2);

    hal_serial_write("BHARAT_IRQ:ROOT_DOMAIN=READY arch=arm64\n");
}

void hal_irq_init_cpu_local(uint32_t cpu_id) {
    // Calculate the per-CPU GICR base (128 KB frame size per CPU in GICv3)
    void *cpu_gicr = (void *)((uintptr_t)g_gicr_base + (uint64_t)cpu_id * 0x20000ULL);

    volatile uint32_t *waker_ptr = (volatile uint32_t *)((uintptr_t)cpu_gicr + GICR_WAKER);
    volatile uint32_t *groupr_ptr = (volatile uint32_t *)((uintptr_t)cpu_gicr + GICR_IGROUPR0);

    // Wake up the redistributor
    uint32_t waker = *waker_ptr;
    waker &= ~2; // Clear ProcessorSleep bit
    *waker_ptr = waker;

    // Wait for ChildrenAsleep to clear
    while ((*waker_ptr) & 4);

    // Group 1 routing for SGIs/PPIs
    *groupr_ptr = 0xFFFFFFFF;
}

void hal_ipi_init_cpu_local(uint32_t cpu_id) {
    (void)cpu_id;
}

int hal_irq_enable(uint32_t vector) {
    if (vector >= 1024) return -1;
    if (vector >= 32) {
        uint32_t offset = 0x0100 + (vector / 32) * 4;
        gicd_write(offset, 1U << (vector % 32));
    } else {
        uint32_t cpu_id = hal_cpu_get_id();
        void *cpu_gicr = (void *)((uintptr_t)g_gicr_base + (uint64_t)cpu_id * 0x20000ULL);
        volatile uint32_t *isenabler0 = (volatile uint32_t *)((uintptr_t)cpu_gicr + 0x10100);
        *isenabler0 = 1U << vector;
    }
    return 0;
}

int hal_irq_disable(uint32_t vector) {
    if (vector >= 1024) return -1;
    if (vector >= 32) {
        uint32_t offset = 0x0180 + (vector / 32) * 4;
        gicd_write(offset, 1U << (vector % 32));
    } else {
        uint32_t cpu_id = hal_cpu_get_id();
        void *cpu_gicr = (void *)((uintptr_t)g_gicr_base + (uint64_t)cpu_id * 0x20000ULL);
        volatile uint32_t *icenabler0 = (volatile uint32_t *)((uintptr_t)cpu_gicr + 0x10180);
        *icenabler0 = 1U << vector;
    }
    return 0;
}

int hal_ipi_send(uint32_t cpu_id, uint32_t reason_vector) {
    uint64_t aff1 = (cpu_id >> 8) & 0xFF;
    uint64_t aff0 = cpu_id & 0x0F;
    uint64_t sgi_val = (aff1 << 16) | reason_vector << 24 | (1 << aff0);
    write_icc_sgi1r_el1(sgi_val);
    __asm__ volatile("isb; dsb sy");

    return 0;
}

void hal_irq_eoi(uint32_t vector) {
    write_icc_eoir1_el1((uint64_t)vector);
}
