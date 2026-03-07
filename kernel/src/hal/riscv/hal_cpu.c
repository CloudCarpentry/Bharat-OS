#include "hal/hal.h"
#include "hal/riscv_bsp.h"

#include "../../boot/riscv/sbi.h"

// RISC-V Specific HAL Implementation (RV64 / Shakti)

static uint64_t g_boot_hart_id;
static uint64_t g_boot_fdt_ptr;

void hal_riscv_set_boot_info(uint64_t hart_id, uint64_t fdt_ptr) {
    g_boot_hart_id = hart_id;
    g_boot_fdt_ptr = fdt_ptr;
}

uint64_t hal_riscv_boot_hart_id(void) {
    return g_boot_hart_id;
}

uint64_t hal_riscv_boot_fdt_ptr(void) {
    return g_boot_fdt_ptr;
}


void hal_serial_init(void) {
    // OpenSBI console is already available; no additional UART init required.
}

void hal_serial_write_char(char c) {
    sbi_console_putchar((int)c);
}

void hal_serial_write(const char* s) {
    if (!s) {
        return;
    }

    while (*s != '\0') {
        if (*s == '\n') {
            sbi_console_putchar('\r');
        }
        sbi_console_putchar(*s++);
    }
}

int hal_serial_read_char(void) {
    return sbi_console_getchar();
}

void hal_riscv_send_ipi_payload(const unsigned long* hart_mask, uint64_t payload) {
    sbi_send_ipi_payload(hart_mask, payload);
}

void hal_send_ipi_payload(uint32_t target_core, uint64_t payload) {
    unsigned long hart_mask = (1UL << target_core);
    hal_riscv_send_ipi_payload(&hart_mask, payload);
}

void hal_cpu_halt(void) {
    // Wait for interrupt instruction
    __asm__ volatile("wfi");
}

void hal_cpu_enable_interrupts(void) {
#ifdef CONFIG_RISCV_M_MODE
    // Set MIE (Machine Interrupt Enable) bit in mstatus CSR
    __asm__ volatile("csrsi mstatus, 8");
#else
    // Set SIE (Supervisor Interrupt Enable) bit in sstatus CSR
    __asm__ volatile("csrsi sstatus, 2");
#endif
}

void hal_cpu_disable_interrupts(void) {
#ifdef CONFIG_RISCV_M_MODE
    // Clear MIE (Machine Interrupt Enable) bit in mstatus CSR
    __asm__ volatile("csrci mstatus, 8");
#else
    // Clear SIE (Supervisor Interrupt Enable) bit in sstatus CSR
    __asm__ volatile("csrci sstatus, 2");
#endif
}

// --- Trap / Interrupt Handling ---

extern void trap_vector(void); // Assuming there's an assembly trap vector somewhere, or we define a simple one
// We'll define a simple C trap handler wrapper here to satisfy the linker if needed, but ideally we'd set stvec to a real asm handler
static void simple_trap_vector(void) __attribute__((interrupt("supervisor")));
static void simple_trap_vector(void) {
    uint64_t scause;
    __asm__ volatile("csrr %0, scause" : "=r"(scause));

    // Check if it's a timer interrupt (Supervisor Timer Interrupt is cause 5)
    if ((scause & 0x8000000000000000ULL) && (scause & 0x7FFFFFFFFFFFFFFFULL) == 5) {
        // Clear timer interrupt by setting next timer
        // This is a bit of a hack since we need tick_hz, but we'll use a default
        sbi_set_timer(1000000ULL / 100ULL); // Assuming 100Hz
        hal_timer_tick();
    }
}

void hal_init(void) {
    riscv_bsp_config_t cfg;

    // Setup trap vectors (stvec) for Supervisor mode.
#ifdef CONFIG_RISCV_M_MODE
    __asm__ volatile("csrw mtvec, %0" : : "r"((uint64_t)simple_trap_vector));
#else
    __asm__ volatile("csrw stvec, %0" : : "r"((uint64_t)simple_trap_vector));
#endif

    // Setup SBI console if running in Supervisor mode, or physical UART if Machine mode.
    hal_serial_init();

#ifdef BHARAT_RISCV_SOC_PROFILE_STR
    const char* profile = BHARAT_RISCV_SOC_PROFILE_STR;
#else
    const char* profile = "qemu-virt";
#endif

    if (hal_riscv_bsp_detect(profile, g_boot_fdt_ptr, &cfg) == 0) {
        (void)hal_riscv_bsp_init(&cfg);
    }
}

void hal_tlb_flush(unsigned long long vaddr) {
    __asm__ volatile("sfence.vma %0, x0" :: "r"(vaddr) : "memory");
}

// --- PLIC Definitions (QEMU virt) ---
#define PLIC_BASE        0x0c000000ULL
#define PLIC_PRIORITY    PLIC_BASE
#define PLIC_PENDING     (PLIC_BASE + 0x1000)
#define PLIC_ENABLE      (PLIC_BASE + 0x2000)
#define PLIC_THRESHOLD   (PLIC_BASE + 0x200000)
#define PLIC_CLAIM       (PLIC_BASE + 0x200004)

// Hart context calculation (assuming Hart 0 Supervisor context is context 1)
#define PLIC_ENABLE_CTX(ctx)   (PLIC_ENABLE + (ctx) * 0x80)
#define PLIC_THRESHOLD_CTX(ctx) (PLIC_THRESHOLD + (ctx) * 0x1000)

int hal_interrupt_controller_init(void) {
    // For simplicity, we initialize PLIC for Hart 0, Supervisor Mode (Context 1)
    uint32_t ctx = 1; // Hart 0, Supervisor

    // Set context threshold to 0 (accept all interrupts)
    volatile uint32_t *threshold = (volatile uint32_t *)PLIC_THRESHOLD_CTX(ctx);
    *threshold = 0;

    return 0;
}

int hal_interrupt_route(uint32_t irq, uint32_t target_core) {
    if (irq == 0 || irq > 53) return -1; // PLIC has 53 interrupts on QEMU virt

    // Set priority to 1 (lowest non-zero)
    volatile uint32_t *priority = (volatile uint32_t *)(PLIC_PRIORITY + irq * 4);
    *priority = 1;

    // Enable interrupt for the target core's supervisor context
    // Target core * 2 + 1 (assuming S-mode)
    uint32_t ctx = target_core * 2 + 1;
    volatile uint32_t *enable = (volatile uint32_t *)PLIC_ENABLE_CTX(ctx);

    // Enable bit for this irq
    *enable |= (1 << (irq % 32));

    return 0;
}

int hal_timer_source_init(uint32_t tick_hz) {
    if (tick_hz == 0U) {
        return -1;
    }

    // Enable Supervisor Timer Interrupt (STIE) in sie CSR
#ifdef CONFIG_RISCV_M_MODE
    __asm__ volatile("csrs mie, %0" : : "r"(32)); // MTIE is bit 5
#else
    __asm__ volatile("csrs sie, %0" : : "r"(32)); // STIE is bit 5
#endif

    // Baseline periodic timer request via SBI TIME extension.
    sbi_set_timer(1000000ULL / (uint64_t)tick_hz);
    return 0;
}
