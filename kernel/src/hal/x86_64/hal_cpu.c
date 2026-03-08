#include "hal/hal.h"
#include "advanced/ai_sched.h"

#include <stdint.h>

// x86_64 Specific HAL Implementation

#define COM1_PORT 0x3F8

static inline uint8_t x86_inb(uint16_t port) {
    uint8_t value;
    __asm__ volatile("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static inline void x86_outb(uint16_t port, uint8_t value) {
    __asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

void hal_serial_init(void) {
    // Disable interrupts
    x86_outb(COM1_PORT + 1, 0x00);
    // Enable DLAB
    x86_outb(COM1_PORT + 3, 0x80);
    // Set baud divisor to 3 (38400 baud)
    x86_outb(COM1_PORT + 0, 0x03);
    x86_outb(COM1_PORT + 1, 0x00);
    // 8 bits, no parity, one stop bit
    x86_outb(COM1_PORT + 3, 0x03);
    // Enable FIFO, clear them, with 14-byte threshold
    x86_outb(COM1_PORT + 2, 0xC7);
    // IRQs enabled, RTS/DSR set
    x86_outb(COM1_PORT + 4, 0x0B);
}

void hal_serial_write_char(char c) {
    // Wait for transmit holding register empty
    while ((x86_inb(COM1_PORT + 5) & 0x20U) == 0U) {
    }

    x86_outb(COM1_PORT, (uint8_t)c);
}

void hal_serial_write(const char* s) {
    if (!s) {
        return;
    }

    while (*s != '\0') {
        if (*s == '\n') {
            hal_serial_write_char('\r');
        }
        hal_serial_write_char(*s++);
    }
}

// Added to intercept serial writes if needed by generic KPRINT
void serial_puts(const char* s) {
    hal_serial_write(s);
}

int hal_serial_read_char(void) {
    // Data ready bit
    if ((x86_inb(COM1_PORT + 5) & 0x01U) == 0U) {
        return -1;
    }
    return (int)x86_inb(COM1_PORT);
}

void hal_cpu_halt(void) {
    // Inject x86 assembly for HLT instruction
    __asm__ volatile("hlt");
}

void hal_cpu_reboot(void) {
    // Attempt a reboot using 8042 keyboard controller
    x86_outb(0x64, 0xFE);
    while (1) {
        __asm__ volatile("hlt");
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
    uint64_t cr2, cr3, rbp, rsp;
    __asm__ volatile("mov %%cr2, %0" : "=r"(cr2));
    __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
    __asm__ volatile("mov %%rbp, %0" : "=r"(rbp));
    __asm__ volatile("mov %%rsp, %0" : "=r"(rsp));

    hal_serial_write("\n--- x86_64 CPU State Dump ---\n");
    hal_serial_write("CR2: "); print_hex(cr2); hal_serial_write("\n");
    hal_serial_write("CR3: "); print_hex(cr3); hal_serial_write("\n");
    hal_serial_write("RSP: "); print_hex(rsp); hal_serial_write("\n");
    hal_serial_write("RBP: "); print_hex(rbp); hal_serial_write("\n");

    hal_serial_write("\nStack Trace (Frame Pointers):\n");
    uint64_t current_rbp = rbp;
    int depth = 0;
    while (current_rbp != 0 && current_rbp >= 0x1000 && depth < 10) {
        uint64_t* frame = (uint64_t*)current_rbp;
        uint64_t next_rbp = frame[0];
        uint64_t ret_addr = frame[1];

        hal_serial_write("  [");
        char depth_str[2] = {(char)('0' + depth), '\0'};
        hal_serial_write(depth_str);
        hal_serial_write("] pc="); print_hex(ret_addr);
        hal_serial_write(" fp="); print_hex(next_rbp); hal_serial_write("\n");

        if (next_rbp <= current_rbp) {
            break; // Stop if frame pointer is not strictly increasing
        }
        current_rbp = next_rbp;
        depth++;
    }
    hal_serial_write("-----------------------------\n");
}

void hal_cpu_enable_interrupts(void) {
    __asm__ volatile("sti");
}

void hal_cpu_disable_interrupts(void) {
    __asm__ volatile("cli");
}

// --- IDT Definitions ---
struct idt_entry {
    uint16_t isr_low;
    uint16_t kernel_cs;
    uint8_t  ist;
    uint8_t  attributes;
    uint16_t isr_mid;
    uint32_t isr_high;
    uint32_t reserved;
} __attribute__((packed));

struct idtr {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

static struct idt_entry idt[256];
static struct idtr idtr;

static void idt_set_descriptor(uint8_t vector, void *isr, uint8_t flags) {
    struct idt_entry *descriptor = &idt[vector];
    descriptor->isr_low    = (uint64_t)isr & 0xFFFF;
    descriptor->kernel_cs  = 0x08; // Assuming 0x08 is kernel code segment
    descriptor->ist        = 0;
    descriptor->attributes = flags;
    descriptor->isr_mid    = ((uint64_t)isr >> 16) & 0xFFFF;
    descriptor->isr_high   = ((uint64_t)isr >> 32) & 0xFFFFFFFF;
    descriptor->reserved   = 0;
}

static void default_isr(void) {
    __asm__ volatile("iretq");
}

static void default_timer_isr(void) {
    // Ack APIC EOI
    volatile uint32_t *apic_eoi = (volatile uint32_t *)0xFEE000B0;
    *apic_eoi = 0;

    // Call generic timer tick
    hal_timer_tick();

    __asm__ volatile("iretq");
}


// --- APIC Definitions ---
#define APIC_BASE        0xFEE00000
#define APIC_SIVR        0xFEE000F0
#define APIC_ICR_LOW     0xFEE00300
#define APIC_ICR_HIGH    0xFEE00310
#define APIC_LVT_TIMER   0xFEE00320
#define APIC_TMRDIV      0xFEE003E0
#define APIC_TMRINITCNT  0xFEE00380
#define APIC_TMRCURRCNT  0xFEE00390
#define APIC_EOI         0xFEE000B0

// --- IOAPIC Definitions ---
#define IOAPIC_BASE      0xFEC00000
#define IOAPIC_REG_SEL   (IOAPIC_BASE + 0x00)
#define IOAPIC_REG_WIN   (IOAPIC_BASE + 0x10)

static inline void ioapic_write(uint8_t offset, uint32_t val) {
    *(volatile uint32_t*)IOAPIC_REG_SEL = offset;
    *(volatile uint32_t*)IOAPIC_REG_WIN = val;
}


void hal_init(void) {
    // Setup IDT, GDT for x86_64
    hal_serial_init();

    // Initialize IDT to default handler
    for (int i = 0; i < 256; i++) {
        idt_set_descriptor(i, default_isr, 0x8E); // 0x8E: Present, Ring 0, Interrupt Gate
    }

    // Timer vector 32
    idt_set_descriptor(32, default_timer_isr, 0x8E);

    idtr.base = (uint64_t)&idt[0];
    idtr.limit = (uint16_t)sizeof(struct idt_entry) * 256 - 1;

    __asm__ volatile("lidt %0" : : "m"(idtr));
}

void hal_tlb_flush(unsigned long long vaddr) {
    __asm__ volatile("invlpg (%0)" :: "r"(vaddr) : "memory");
}

void hal_send_ipi_payload(uint32_t target_core, uint64_t payload) {
    (void)payload; // We can't actually send payload via bare IPI without URPC, just send IPI

    volatile uint32_t *apic_icr_high = (volatile uint32_t *)APIC_ICR_HIGH;
    volatile uint32_t *apic_icr_low  = (volatile uint32_t *)APIC_ICR_LOW;

    // Send fixed IPI (vector 250) to target core
    *apic_icr_high = (target_core << 24);
    *apic_icr_low = 0x00004000 | 250; // Fixed delivery, assert, edge trigger, vector 250
}


int hal_interrupt_controller_init(void) {
    // Enable local APIC (set Spurious Interrupt Vector Register)
    volatile uint32_t *apic_sivr = (volatile uint32_t *)APIC_SIVR;
    *apic_sivr = 0x1FF; // Enable APIC and set spurious vector to 0xFF

    // Disable 8259 PICs to ensure only APIC is used
    x86_outb(0xA1, 0xFF);
    x86_outb(0x21, 0xFF);

    return 0;
}

int hal_interrupt_route(uint32_t irq, uint32_t target_core) {
    if (irq > 23) return -1; // Assuming standard 24-pin IOAPIC

    // Route IRQ to vector (irq + 32) on target core
    uint8_t vector = (uint8_t)(irq + 32);

    // REDTBL offset starts at 0x10. Each entry is 64 bits (2 * 32-bit registers)
    uint8_t ioapic_reg = 0x10 + (uint8_t)(irq * 2);

    // Lower 32 bits: vector, fixed delivery, unmasked
    uint32_t low = vector;
    // Upper 32 bits: target APIC ID
    uint32_t high = target_core << 24;

    ioapic_write(ioapic_reg, low);
    ioapic_write(ioapic_reg + 1, high);

    return 0;
}

int hal_timer_source_init(uint32_t tick_hz) {
    if (tick_hz == 0) return -1;

    // Set APIC timer divider to 16
    volatile uint32_t *apic_tmrdiv = (volatile uint32_t *)APIC_TMRDIV;
    *apic_tmrdiv = 0x03;

    // Assume APIC bus frequency is around 1GHz for simplicity in bare-metal stub
    // and divide by tick_hz. A real implementation would calibrate via PIT.
    uint32_t initial_count = 1000000000 / 16 / tick_hz;

    // Configure timer in periodic mode, vector 32
    volatile uint32_t *apic_lvt_timer = (volatile uint32_t *)APIC_LVT_TIMER;
    *apic_lvt_timer = 32 | 0x20000; // Vector 32, Periodic mode (bit 17)

    // Set initial count
    volatile uint32_t *apic_tmrinitcnt = (volatile uint32_t *)APIC_TMRINITCNT;
    *apic_tmrinitcnt = initial_count;

    return 0;
}

uint32_t hal_cpu_get_id(void) {
    // Return Local APIC ID (using CPUID for simplicity here)
    uint32_t ebx;
    __asm__ volatile("cpuid" : "=b"(ebx) : "a"(1) : "ecx", "edx");
    return ebx >> 24;
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

    // Read rdtsc
    uint32_t low, high;
    __asm__ volatile("rdtsc" : "=a"(low), "=d"(high));
    cycles = ((uint64_t)high << 32) | low;

    uint64_t cycles_delta = cycles - g_pmc_state[thread_id].last_cycles;

    // TODO: Program IA32_PERF_GLOBAL_CTRL and use rdpmc(1)
    uint64_t instr_delta = cycles_delta / 2;

    out_sample->available = 1U;
    out_sample->cycles_delta = cycles_delta;
    out_sample->instructions_delta = instr_delta;

    g_pmc_state[thread_id].last_cycles = cycles;
    // We mock instructions here
    g_pmc_state[thread_id].last_instr += instr_delta;

    return 0;
}
