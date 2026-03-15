#include "hal/hal_irq.h"
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
    if (!g_its_base) return -1;

    // Simplistic stub allocating LPIs
    for (int i = 0; i < count; i++) {
        desc_array[i].irq = 8192 + i; // LPI base is 8192
        desc_array[i].msg.address = (uint64_t)(uintptr_t)g_its_base + 0x0040; // GITS_TRANSLATER
        desc_array[i].msg.data = desc_array[i].irq; // The EventID
    }
    return 0;
}

static void its_free_msi(msi_domain_t* domain, msi_desc_t* desc_array, int count) {
    (void)domain;
    (void)desc_array;
    (void)count;
    // Release LPIs
}

static msi_domain_t its_msi_domain = {
    .name = "gicv3-its",
    .base_domain = NULL,
    .alloc_msi = its_alloc_msi,
    .free_msi = its_free_msi,
    .write_msg = NULL,
    .host_data = NULL
};

int hal_irq_init_boot(void) {
    system_discovery_t* disc = hal_get_system_discovery();
    if (disc) {
        for (uint32_t i = 0; i < disc->irq_ctrl_count; i++) {
            if (disc->irq_ctrls[i].type == IRQ_CTRL_GICV3) {
                g_gicd_base = (void*)(uintptr_t)disc->irq_ctrls[i].base;
                g_gicr_base = (void*)(uintptr_t)disc->irq_ctrls[i].aux_base;
            } else if (disc->irq_ctrls[i].type == IRQ_CTRL_GIC_ITS) {
                g_its_base = (void*)(uintptr_t)disc->irq_ctrls[i].base;
                msi_domain_register(&its_msi_domain);
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
    return 0;
}

int hal_irq_init_cpu_local(uint32_t cpu_id) {
    (void)cpu_id;
    // Wake up the redistributor
    uint32_t waker = gicr_read(GICR_WAKER);
    waker &= ~2; // Clear ProcessorSleep bit
    gicr_write(GICR_WAKER, waker);
    // Wait for ChildrenAsleep to clear
    while (gicr_read(GICR_WAKER) & 4);

    // Group 1 routing for SGIs/PPIs
    gicr_write(GICR_IGROUPR0, 0xFFFFFFFF);
    return 0;
}

int hal_irq_enable(uint32_t vector) {
    // Unmask logic
    return 0;
}

int hal_irq_disable(uint32_t vector) {
    // Mask logic
    return 0;
}

int hal_ipi_send(uint32_t cpu_id, uint32_t reason_vector) {
    // Send SGI via system register
    uint64_t aff1 = (cpu_id >> 8) & 0xFF;
    uint64_t aff0 = cpu_id & 0x0F; // Target list is only 16 bits in SGI1R, bounded to 0-15
    uint64_t sgi_val = (aff1 << 16) | reason_vector << 24 | (1 << aff0);
    write_icc_sgi1r_el1(sgi_val);
    __asm__ volatile("isb; dsb sy");
    return 0;
}

void hal_irq_eoi(uint32_t vector) {
    write_icc_eoir1_el1(vector);
}
