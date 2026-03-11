#include "hal/hal_irq.h"
#include "hal/hal_boot.h"

// Define registers for basic GICv3 operations
#define GICD_CTLR        0x0000
#define GICD_IGROUPR0    0x0080
#define GICR_WAKER       0x0014
#define GICR_IGROUPR0    0x0080

// ICC System Registers mapped via generic sysreg macros
#define read_icc_iar1_el1(val)  __asm__ volatile("mrs %0, S3_0_C12_C12_0" : "=r"(val))
#define write_icc_eoir1_el1(val) __asm__ volatile("msr S3_0_C12_C12_1, %0" : : "r"(val))
#define write_icc_sgi1r_el1(val) __asm__ volatile("msr S3_0_C12_C11_5, %0" : : "r"(val))

static void* g_gicd_base = (void*)0x08000000;
static void* g_gicr_base = (void*)0x080A0000;

static inline void gicd_write(uint32_t offset, uint32_t value) {
    *(volatile uint32_t*)((uint64_t)g_gicd_base + offset) = value;
}

static inline void gicr_write(uint32_t offset, uint32_t value) {
    *(volatile uint32_t*)((uint64_t)g_gicr_base + offset) = value;
}

static inline uint32_t gicr_read(uint32_t offset) {
    return *(volatile uint32_t*)((uint64_t)g_gicr_base + offset);
}

int hal_irq_init_boot(void) {
    // Disable Distributor before config
    gicd_write(GICD_CTLR, 0);
    // Route interrupts to non-secure Group 1
    gicd_write(GICD_IGROUPR0, 0xFFFFFFFF);
    // Enable Group 1 interrupts
    gicd_write(GICD_CTLR, 2);
    return 0;
}

int hal_irq_init_cpu_local(void) {
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
