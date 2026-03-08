#include "hal/hal.h"
#include "hal/riscv_bsp.h"
#include "advanced/ai_sched.h"

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

void hal_cpu_reboot(void) {
    sbi_system_reset(0, 0);
    while (1) {
        __asm__ volatile("wfi");
    }
}

static void print_hex(uint64_t val) {
    char buf[17];
    buf[16] = '\0';
    for (int i = 15; i >= 0; i--) {
        uint8_t nibble = val & 0xF;
        buf[i] = nibble < 10 ? '0' + nibble : 'a' + (nibble - 10);
        val >>= 4;
    }
    hal_serial_write("0x");
    hal_serial_write(buf);
}

void hal_cpu_dump_state(void) {
    uint64_t sepc, scause, stval, sstatus, fp, sp;
    __asm__ volatile("csrr %0, sepc" : "=r"(sepc));
    __asm__ volatile("csrr %0, scause" : "=r"(scause));
    __asm__ volatile("csrr %0, stval" : "=r"(stval));
    __asm__ volatile("csrr %0, sstatus" : "=r"(sstatus));
    __asm__ volatile("mv %0, s0" : "=r"(fp));
    __asm__ volatile("mv %0, sp" : "=r"(sp));

    hal_serial_write("\n--- RISC-V CPU State Dump ---\n");
    hal_serial_write("SEPC: "); print_hex(sepc); hal_serial_write("\n");
    hal_serial_write("SCAUSE: "); print_hex(scause); hal_serial_write("\n");
    hal_serial_write("STVAL: "); print_hex(stval); hal_serial_write("\n");
    hal_serial_write("SSTATUS: "); print_hex(sstatus); hal_serial_write("\n");
    hal_serial_write("FP: "); print_hex(fp); hal_serial_write("\n");
    hal_serial_write("SP: "); print_hex(sp); hal_serial_write("\n");

    hal_serial_write("\nStack Trace (Frame Pointers):\n");
    uint64_t current_fp = fp;
    int depth = 0;
    while (current_fp != 0 && current_fp >= 0x1000 && depth < 10) {
        uint64_t* frame = (uint64_t*)(current_fp - 16); // Previous fp is at fp-16, ra at fp-8
        uint64_t next_fp = frame[0];
        uint64_t ret_addr = frame[1];

        hal_serial_write("  [");
        char depth_str[2] = {(char)('0' + depth), '\0'};
        hal_serial_write(depth_str);
        hal_serial_write("] pc="); print_hex(ret_addr);
        hal_serial_write(" fp="); print_hex(next_fp); hal_serial_write("\n");

        if (next_fp <= current_fp) {
            break; // Stop if frame pointer is not strictly increasing
        }
        current_fp = next_fp;
        depth++;
    }
    hal_serial_write("-----------------------------\n");
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

extern void trap_entry(void);

void hal_init(void) {
    riscv_bsp_config_t cfg;

    // Setup trap vectors (stvec) for Supervisor mode.
#ifdef CONFIG_RISCV_M_MODE
    __asm__ volatile("csrw mtvec, %0" : : "r"((uint64_t)trap_entry));
#else
    __asm__ volatile("csrw stvec, %0" : : "r"((uint64_t)trap_entry));
    // Set sscratch to 0 to indicate we are initially in S-mode
    __asm__ volatile("csrw sscratch, 0");
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

static uint64_t g_timer_interval;

int hal_timer_source_init(uint32_t tick_hz) {
    if (tick_hz == 0U) {
        return -1;
    }

    // Assuming a timebase frequency of 10MHz (e.g. QEMU Virt and Shakti default)
    uint64_t timebase_freq = 10000000ULL;
    g_timer_interval = timebase_freq / (uint64_t)tick_hz;

    uint64_t current_time;
    __asm__ volatile("csrr %0, time" : "=r"(current_time));
    sbi_set_timer(current_time + g_timer_interval);

    // Enable Supervisor Timer Interrupt (STIE) in sie CSR
#ifdef CONFIG_RISCV_M_MODE
    __asm__ volatile("csrs mie, %0" : : "r"(32)); // MTIE is bit 5
#else
    __asm__ volatile("csrs sie, %0" : : "r"(32)); // STIE is bit 5
#endif

    return 0;
}

void hal_timer_isr(void) {
    uint64_t current_time;
    __asm__ volatile("csrr %0, time" : "=r"(current_time));

    // Set next timer interrupt
    sbi_set_timer(current_time + g_timer_interval);

    hal_timer_tick();
}

uint32_t hal_cpu_get_id(void) {
    // Read mhartid (assuming machine mode or standard supervisor mode access via OpenSBI/SBI)
    // Here we'll just return the boot hart id for simplicity. Ideally we read sscratch or use sbi.
    return (uint32_t)g_boot_hart_id;
}

#define SCHED_MAX_THREADS 64U

typedef struct {
    uint64_t last_cycles;
    uint64_t last_instr;
} pmc_state_t;

static pmc_state_t g_pmc_state[SCHED_MAX_THREADS] = {0};

int ai_sched_arch_sample_pmc(uint32_t thread_id, ai_pmc_sample_t* out_sample) {
    if (!out_sample) {
        return -1;
    }

    if (thread_id >= SCHED_MAX_THREADS) {
        return -1;
    }

    uint64_t cycles = 0;
    uint64_t instr = 0;

    __asm__ volatile("csrr %0, cycle" : "=r"(cycles));
    __asm__ volatile("csrr %0, instret" : "=r"(instr));

    out_sample->available = 1U;
    out_sample->cycles_delta = cycles - g_pmc_state[thread_id].last_cycles;
    out_sample->instructions_delta = instr - g_pmc_state[thread_id].last_instr;

    g_pmc_state[thread_id].last_cycles = cycles;
    g_pmc_state[thread_id].last_instr = instr;

    return 0;
}
