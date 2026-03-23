#include "hal/hal_irq.h"
#include "hal/hal_boot.h"
#include "hal/hal.h"

// Define PLIC base address and offsets
#define PLIC_BASE        0x0C000000
#define PLIC_PRIORITY    0x000000
#define PLIC_PENDING     0x001000
#define PLIC_ENABLE      0x002000
#define PLIC_THRESHOLD   0x200000
#define PLIC_CLAIM       0x200004

// Provide SBI definitions since Shakti uses them
struct sbiret {
    long error;
    long value;
};
#define SBI_EXT_IPI 0x735049
#define SBI_EXT_IPI_SEND_IPI 0

static inline struct sbiret sbi_ecall(int ext, int fid, unsigned long arg0,
                                      unsigned long arg1, unsigned long arg2,
                                      unsigned long arg3, unsigned long arg4,
                                      unsigned long arg5) {
    struct sbiret ret;
    register unsigned long a0 asm("a0") = arg0;
    register unsigned long a1 asm("a1") = arg1;
    register unsigned long a2 asm("a2") = arg2;
    register unsigned long a3 asm("a3") = arg3;
    register unsigned long a4 asm("a4") = arg4;
    register unsigned long a5 asm("a5") = arg5;
    register unsigned long a6 asm("a6") = fid;
    register unsigned long a7 asm("a7") = ext;
    asm volatile("ecall"
                 : "+r"(a0), "+r"(a1)
                 : "r"(a2), "r"(a3), "r"(a4), "r"(a5), "r"(a6), "r"(a7)
                 : "memory");
    ret.error = a0;
    ret.value = a1;
    return ret;
}

static inline void plic_write(uint32_t offset, uint32_t value) {
    *(volatile uint32_t*)((uint64_t)PLIC_BASE + offset) = value;
}

static inline uint32_t plic_read(uint32_t offset) {
    return *(volatile uint32_t*)((uint64_t)PLIC_BASE + offset);
}

int hal_irq_init_boot(void) {
    // Set priority of IRQ 1..1023 to 0
    for (int i = 1; i < 1024; i++) {
        plic_write(PLIC_PRIORITY + (i * 4), 1);
    }
    return 0;
}

int hal_irq_init_cpu_local(void) {
    uint32_t core_id = hal_cpu_get_id();
    uint32_t hart_id = hal_boot_get_info()->cpus[core_id].cpu_id;
    // Set Hart threshold to 0 to accept all interrupts
    uint32_t target = hart_id * 2 + 1; // Hart N S-Mode
    plic_write(PLIC_THRESHOLD + (target * 0x1000), 0);
    // Enable external, timer and software interrupts in mie CSR
    __asm__ volatile("csrs mie, %0" : : "r"(0x222));
    return 0;
}

int hal_irq_enable(uint32_t vector) {
    uint32_t core_id = hal_cpu_get_id();
    uint32_t hart_id = hal_boot_get_info()->cpus[core_id].cpu_id;
    uint32_t target = hart_id * 2 + 1;
    uint32_t word = vector / 32;
    uint32_t bit = vector % 32;

    uint32_t val = plic_read(PLIC_ENABLE + (target * 0x80) + (word * 4));
    plic_write(PLIC_ENABLE + (target * 0x80) + (word * 4), val | (1 << bit));
    return 0;
}

int hal_irq_disable(uint32_t vector) {
    uint32_t core_id = hal_cpu_get_id();
    uint32_t hart_id = hal_boot_get_info()->cpus[core_id].cpu_id;
    uint32_t target = hart_id * 2 + 1;
    uint32_t word = vector / 32;
    uint32_t bit = vector % 32;

    uint32_t val = plic_read(PLIC_ENABLE + (target * 0x80) + (word * 4));
    plic_write(PLIC_ENABLE + (target * 0x80) + (word * 4), val & ~(1 << bit));
    return 0;
}

int hal_ipi_send(uint32_t cpu_id, uint32_t reason_vector) {
    if (cpu_id >= 64) {
        // Fallback for large systems not currently supported in mask
        return -1;
    }
    unsigned long mask = 1UL << cpu_id;
    sbi_ecall(SBI_EXT_IPI, SBI_EXT_IPI_SEND_IPI, mask, 0, 0, 0, 0, 0);
    return 0;
}

void hal_irq_eoi(uint32_t vector) {
    uint32_t core_id = hal_cpu_get_id();
    uint32_t hart_id = hal_boot_get_info()->cpus[core_id].cpu_id;
    uint32_t target = hart_id * 2 + 1;
    plic_write(PLIC_CLAIM + (target * 0x1000), vector);
}
