#include "hal/hal_timer.h"
#include "sched/ai_sched.h"
#include "hal/hal.h"
#include "console/console_core.h"
#include "secure_boot.h"

#include <stdint.h>
#if __has_include("bharat_config.h")
#include "bharat_config.h"
#endif

#ifndef MAX_SUPPORTED_CORES
#define MAX_SUPPORTED_CORES 8U
#endif

int hal_secure_boot_arch_check(const bharat_boot_policy_t *policy) {
  if (!policy) {
    return -1;
  }

  if (policy->security_level == BHARAT_BOOT_SECURITY_ENFORCED) {
#if !defined(BHARAT_PLATFORM_ACPI)
    return -2;
#endif
  }

  return 0;
}

void hal_core_notify(uint32_t target_core, uint64_t payload_or_reason) {
  hal_send_ipi_payload(target_core, payload_or_reason);
}

void hal_core_wait_event(void) {
  __asm__ volatile("hlt" : : : "memory");
}

void hal_core_poll_event(void) {
  __asm__ volatile("pause" : : : "memory");
}

// x86_64 Specific HAL Implementation

static void hal_cpu_pmc_init(void);

static inline void x86_outb(uint16_t port, uint8_t value) {
  __asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline void x86_wrmsr(uint32_t msr, uint64_t value) {
  uint32_t low = (uint32_t)value;
  uint32_t high = (uint32_t)(value >> 32);
  __asm__ volatile("wrmsr" : : "c"(msr), "a"(low), "d"(high));
}

inline uint64_t x86_rdpmc(uint32_t counter) {
  uint32_t low, high;
  __asm__ volatile("rdpmc" : "=a"(low), "=d"(high) : "c"(counter));
  return ((uint64_t)high << 32) | low;
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

// TODO: Needs refactor: #include directive placed mid-file for dependency/order compatibility.
#include "trap.h"

bool hal_cpu_is_syscall(const void *trap_frame) {
    if (!trap_frame) return false;
    const trap_frame_t *tf = (const trap_frame_t *)trap_frame;
    return (tf->cause == 0x80U); // x86_64 uses INT 0x80 for legacy or custom
}

bool hal_cpu_is_page_fault(const void *trap_frame) {
    if (!trap_frame) return false;
    const trap_frame_t *tf = (const trap_frame_t *)trap_frame;
    return (tf->cause == 14); // #PF
}

bool hal_cpu_is_access_fault(const void *trap_frame) {
    (void)trap_frame;
    return false;
}

bool hal_cpu_is_fp_simd_fault(const void *trap_frame) {
    if (!trap_frame) return false;
    const trap_frame_t *tf = (const trap_frame_t *)trap_frame;
    return (tf->cause == 7 || tf->cause == 16 || tf->cause == 19); // #NM, #MF, #XF
}

bool hal_cpu_is_illegal_instruction(const void *trap_frame) {
    if (!trap_frame) return false;
    const trap_frame_t *tf = (const trap_frame_t *)trap_frame;
    return (tf->cause == 6); // #UD
}

uint32_t hal_interrupt_get_active_irq(uint64_t hw_cause) {
    return (uint32_t)hw_cause;
}

uint64_t hal_irq_timer_vector(void) {
    return 32U;
}

uint64_t hal_cpu_get_fault_address(const void *trap_frame) {
    (void)trap_frame;
    uint64_t cr2;
    __asm__ volatile("mov %%cr2, %0" : "=r"(cr2));
    return cr2;
}

__attribute__((weak)) void hal_cpu_dump_trap_frame(const void *trap_frame) {
  if (!trap_frame) {
    return;
  }
  const trap_frame_t *tf = (const trap_frame_t *)trap_frame;
  hal_serial_write("\n--- x86_64 Trap Frame Dump ---\n");
  hal_serial_write("CAUSE: ");
  hal_serial_write_hex(tf->cause);
  hal_serial_write("\n");
  hal_serial_write("PC:    ");
  hal_serial_write_hex(tf->pc);
  hal_serial_write("\n");
  hal_serial_write("SP:    ");
  hal_serial_write_hex(tf->sp);
  hal_serial_write("\n");
  // Dump some general registers, usually gpr[0]-gpr[5] hold arg0-arg5 in syscalls
  hal_serial_write("RAX:   "); hal_serial_write_hex(tf->gpr[0]); hal_serial_write("\n");
  hal_serial_write("RDI:   "); hal_serial_write_hex(tf->gpr[1]); hal_serial_write("\n");
  hal_serial_write("RSI:   "); hal_serial_write_hex(tf->gpr[2]); hal_serial_write("\n");
  hal_serial_write("RDX:   "); hal_serial_write_hex(tf->gpr[3]); hal_serial_write("\n");
  hal_serial_write("RCX:   "); hal_serial_write_hex(tf->gpr[4]); hal_serial_write("\n");
  hal_serial_write("R8:    "); hal_serial_write_hex(tf->gpr[5]); hal_serial_write("\n");
  hal_serial_write("R9:    "); hal_serial_write_hex(tf->gpr[6]); hal_serial_write("\n");
  hal_serial_write("------------------------------\n");
}

void hal_cpu_dump_state(void) {
  uint64_t cr2, cr3, rbp, rsp;
  __asm__ volatile("mov %%cr2, %0" : "=r"(cr2));
  __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
  __asm__ volatile("mov %%rbp, %0" : "=r"(rbp));
  __asm__ volatile("mov %%rsp, %0" : "=r"(rsp));

  hal_serial_write("\n--- x86_64 CPU State Dump ---\n");
  hal_serial_write("CR2: ");
  hal_serial_write_hex(cr2);
  hal_serial_write("\n");
  hal_serial_write("CR3: ");
  hal_serial_write_hex(cr3);
  hal_serial_write("\n");
  hal_serial_write("RSP: ");
  hal_serial_write_hex(rsp);
  hal_serial_write("\n");
  hal_serial_write("RBP: ");
  hal_serial_write_hex(rbp);
  hal_serial_write("\n");

  hal_serial_write("\nStack Trace (Frame Pointers):\n");
  uint64_t current_rbp = rbp;
  int depth = 0;
  while (current_rbp != 0 && current_rbp >= 0x1000 && depth < 10) {
    uint64_t *frame = (uint64_t *)current_rbp;
    uint64_t next_rbp = frame[0];
    uint64_t ret_addr = frame[1];

    hal_serial_write("  [");
    char depth_str[2] = {(char)('0' + depth), '\0'};
    hal_serial_write(depth_str);
    hal_serial_write("] pc=");
    hal_serial_write_hex(ret_addr);
    hal_serial_write(" fp=");
    hal_serial_write_hex(next_rbp);
    hal_serial_write("\n");

    if (next_rbp <= current_rbp) {
      break; // Stop if frame pointer is not strictly increasing
    }
    current_rbp = next_rbp;
    depth++;
  }
  hal_serial_write("-----------------------------\n");
}

void hal_cpu_enable_interrupts(void) { __asm__ volatile("sti"); }

void hal_cpu_disable_interrupts(void) { __asm__ volatile("cli"); }

// --- IDT / TSS Definitions ---

struct tss_entry_struct {
  uint32_t reserved0;
  uint64_t rsp0;
  uint64_t rsp1;
  uint64_t rsp2;
  uint64_t reserved1;
  uint64_t ist1;
  uint64_t ist2;
  uint64_t ist3;
  uint64_t ist4;
  uint64_t ist5;
  uint64_t ist6;
  uint64_t ist7;
  uint64_t reserved2;
  uint16_t reserved3;
  uint16_t iopb_offset;
} __attribute__((packed));

typedef struct tss_entry_struct tss_entry_t;

static tss_entry_t g_tss[MAX_SUPPORTED_CORES] = {0};
static uint8_t g_emergency_stacks[MAX_SUPPORTED_CORES][4096]
    __attribute__((aligned(16)));

extern uint8_t g_per_core_stacks[][16384];

struct idt_entry {
  uint16_t isr_low;
  uint16_t kernel_cs;
  uint8_t ist;
  uint8_t attributes;
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
  descriptor->isr_low = (uint64_t)isr & 0xFFFF;
  descriptor->kernel_cs = 0x08; // Assuming 0x08 is kernel code segment
  descriptor->ist = 0;
  descriptor->attributes = flags;
  descriptor->isr_mid = ((uint64_t)isr >> 16) & 0xFFFF;
  descriptor->isr_high = ((uint64_t)isr >> 32) & 0xFFFFFFFF;
  descriptor->reserved = 0;
}

void isr0(void);
void isr1(void);
void isr2(void);
void isr3(void);
void isr4(void);
void isr5(void);
void isr6(void);
void isr7(void);
void isr8(void);
void isr9(void);
void isr10(void);
void isr11(void);
void isr12(void);
void isr13(void);
void isr14(void);
void isr15(void);
void isr16(void);
void isr17(void);
void isr18(void);
void isr19(void);
void isr20(void);
void isr21(void);
void isr22(void);
void isr23(void);
void isr24(void);
void isr25(void);
void isr26(void);
void isr27(void);
void isr28(void);
void isr29(void);
void isr30(void);
void isr31(void);
void isr32(void);
void isr128(void);

static void default_isr(void) { __asm__ volatile("iretq"); }

void hal_timer_isr(void) {
  // Ack APIC EOI
  volatile uint32_t *apic_eoi = (volatile uint32_t *)0xFEE000B0;
  *apic_eoi = 0;

  // Call generic timer tick
  hal_timer_tick();
}

// --- APIC Definitions ---
#define APIC_BASE 0xFEE00000
#define APIC_SIVR 0xFEE000F0
#define APIC_ICR_LOW 0xFEE00300
#define APIC_ICR_HIGH 0xFEE00310
#define APIC_LVT_TIMER 0xFEE00320
#define APIC_TMRDIV 0xFEE003E0
#define APIC_TMRINITCNT 0xFEE00380
#define APIC_TMRCURRCNT 0xFEE00390
#define APIC_EOI 0xFEE000B0

// --- IOAPIC Definitions ---
#define IOAPIC_BASE 0xFEC00000
#define IOAPIC_REG_SEL (IOAPIC_BASE + 0x00)
#define IOAPIC_REG_WIN (IOAPIC_BASE + 0x10)

static inline void ioapic_write(uint8_t offset, uint32_t val) {
  *(volatile uint32_t *)IOAPIC_REG_SEL = offset;
  *(volatile uint32_t *)IOAPIC_REG_WIN = val;
}

// --- GDT Definitions (TSS requires it) ---
static uint64_t g_gdt[8];
static struct {
  uint16_t limit;
  uint64_t base;
} __attribute__((packed)) g_gdtr;

static void gdt_set_descriptor(int index, uint64_t base, uint32_t limit,
                               uint8_t access, uint8_t gran)
    __attribute__((unused));
static void gdt_set_descriptor(int index, uint64_t base, uint32_t limit,
                               uint8_t access, uint8_t gran) {
  if (index < 0 || index >= 8)
    return;
  g_gdt[index] =
      (limit & 0xFFFFULL) | ((base & 0xFFFFFFULL) << 16) |
      ((uint64_t)access << 40) | (((uint64_t)limit & 0xF0000ULL) << 32) |
      (((uint64_t)gran & 0xFULL) << 52) | ((base & 0xFF000000ULL) << 32);
}

static void gdt_set_system_descriptor(int index, uint64_t base, uint32_t limit,
                                      uint8_t access) {
  if (index < 0 || index >= 7)
    return; // Takes 2 slots (16 bytes)
  g_gdt[index] = (limit & 0xFFFFULL) | ((base & 0xFFFFFFULL) << 16) |
                 ((uint64_t)access << 40) |
                 (((uint64_t)limit & 0xF0000ULL) << 32) |
                 ((base & 0xFF000000ULL) << 32);
  g_gdt[index + 1] = (base >> 32);
}

void hal_init(void) {
  hal_serial_init();
  console_write_raw("H0\n", 3);
  // g_x86_64_serial_backend removed. Boot UART is used via early_console abstraction.

  // Mask legacy PIC to avoid conflicts with exceptions (especially INT 8/Double Fault)
  hal_interrupt_controller_init();

  uint32_t core_id = hal_cpu_get_id();
  if (core_id >= MAX_SUPPORTED_CORES)
    core_id = 0; // Safeguard

  g_gdt[0] = 0;
  g_gdt[1] = 0x00af9a000000ffff; // KCode (0x08)
  g_gdt[2] = 0x00af92000000ffff; // KData (0x10)
  g_gdt[3] = 0x00aff2000000ffff; // UData (0x18)
  g_gdt[4] = 0x00affa000000ffff; // UCode (0x20)

  tss_entry_t *tss = &g_tss[core_id];
  tss->rsp0 = (uint64_t)g_per_core_stacks[core_id] + 16384;
  tss->ist1 = (uint64_t)g_emergency_stacks[core_id] + 4096;
  tss->iopb_offset = sizeof(tss_entry_t);

  uint64_t tss_base = (uint64_t)tss;
  gdt_set_system_descriptor(5, tss_base, sizeof(tss_entry_t) - 1, 0x89);

  g_gdtr.base = (uint64_t)&g_gdt[0];
  g_gdtr.limit = sizeof(g_gdt) - 1;

  console_write_raw("H1\n", 3);
  __asm__ volatile("lgdt %0" : : "m"(g_gdtr));

  console_write_raw("H2\n", 3);
  __asm__ volatile("ltr %w0" : : "r"(0x28));

  console_write_raw("H3\n", 3);

  for (int i = 0; i < 256; i++) {
    idt_set_descriptor(i, default_isr, 0x8E);
  }

  idt_set_descriptor(0, isr0, 0x8E);
  idt_set_descriptor(1, isr1, 0x8E);
  idt_set_descriptor(2, isr2, 0x8E);
  idt_set_descriptor(3, isr3, 0x8E);
  idt_set_descriptor(4, isr4, 0x8E);
  idt_set_descriptor(5, isr5, 0x8E);
  idt_set_descriptor(6, isr6, 0x8E);
  idt_set_descriptor(7, isr7, 0x8E);
  idt_set_descriptor(8, isr8, 0x8E);
  idt[8].ist = 1;

  idt_set_descriptor(9, isr9, 0x8E);
  idt_set_descriptor(10, isr10, 0x8E);
  idt_set_descriptor(11, isr11, 0x8E);
  idt_set_descriptor(12, isr12, 0x8E);
  idt_set_descriptor(13, isr13, 0x8E);
  idt_set_descriptor(14, isr14, 0x8E);
  idt[14].ist = 1;  // Use IST1 for page faults (critical exception needs safe stack)
  idt_set_descriptor(15, isr15, 0x8E);
  idt_set_descriptor(16, isr16, 0x8E);
  idt_set_descriptor(17, isr17, 0x8E);
  idt_set_descriptor(18, isr18, 0x8E);
  idt_set_descriptor(19, isr19, 0x8E);
  idt_set_descriptor(20, isr20, 0x8E);
  idt_set_descriptor(21, isr21, 0x8E);
  idt_set_descriptor(22, isr22, 0x8E);
  idt_set_descriptor(23, isr23, 0x8E);
  idt_set_descriptor(24, isr24, 0x8E);
  idt_set_descriptor(25, isr25, 0x8E);
  idt_set_descriptor(26, isr26, 0x8E);
  idt_set_descriptor(27, isr27, 0x8E);
  idt_set_descriptor(28, isr28, 0x8E);
  idt_set_descriptor(29, isr29, 0x8E);
  idt_set_descriptor(30, isr30, 0x8E);
  idt_set_descriptor(31, isr31, 0x8E);
  idt_set_descriptor(32, isr32, 0x8E);

  console_write_raw("H4\n", 3);

  uint64_t cr0, cr4;
  __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
  cr0 &= ~(1ULL << 2);
  cr0 |= (1ULL << 1);
  __asm__ volatile("mov %0, %%cr0" : : "r"(cr0));

  __asm__ volatile("mov %%cr4, %0" : "=r"(cr4));
  cr4 |= (1ULL << 9);
  cr4 |= (1ULL << 10);
  __asm__ volatile("mov %0, %%cr4" : : "r"(cr4));

  console_write_raw("H5\n", 3);
  hal_cpu_pmc_init();

  idt_set_descriptor(128, isr128, 0xEE);

  idtr.base = (uint64_t)&idt[0];
  idtr.limit = (uint16_t)sizeof(struct idt_entry) * 256 - 1;

  console_write_raw("H6\n", 3);
  __asm__ volatile("lidt %0" : : "m"(idtr));
  console_write_raw("H7\n", 3);
}

void hal_tlb_flush(unsigned long long vaddr) {
  __asm__ volatile("invlpg (%0)" ::"r"(vaddr) : "memory");
}

void hal_send_ipi_payload(uint32_t target_core, uint64_t payload) {
  (void)payload; // We can't actually send payload via bare IPI without URPC,
                 // just send IPI

  volatile uint32_t *apic_icr_high = (volatile uint32_t *)APIC_ICR_HIGH;
  volatile uint32_t *apic_icr_low = (volatile uint32_t *)APIC_ICR_LOW;

  // Send fixed IPI (vector 250) to target core
  *apic_icr_high = (target_core << 24);
  *apic_icr_low =
      0x00004000 | 250; // Fixed delivery, assert, edge trigger, vector 250
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
  if (irq > 23)
    return -1; // Assuming standard 24-pin IOAPIC

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

uint32_t hal_interrupt_acknowledge(void) {
  // x86_64 uses IDT vectors; ack is usually handled by the ISR reading APIC.
  return 0;
}

void hal_interrupt_end_of_interrupt(uint32_t irq) {
  (void)irq;
  // APIC EOI is usually handled in the ISR directly.
}

int hal_timer_source_init(uint32_t tick_hz) {
  if (tick_hz == 0)
    return -1;

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

#define MSR_IA32_PERFEVTSEL1 0x187
#define MSR_IA32_PERF_GLOBAL_CTRL 0x38F
#define ARCH_EVENT_INST_RETIRED 0xC0

static void hal_cpu_pmc_init(void) {
  uint32_t eax, ebx, ecx, edx;
  // CPUID leaf 0xA: Architectural Performance Monitoring
  __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(0xA));
  uint8_t version_id = eax & 0xFF;

  if (version_id == 0) {
      return; // PMU not supported (e.g., QEMU TCG)
  }

  // Program PMC1 to count Instructions Retired (Event 0xC0, Umask 0x00)
  // Bit 16: USR, Bit 17: OS, Bit 22: EN
  uint64_t evtsel =
      ARCH_EVENT_INST_RETIRED | (1ULL << 16) | (1ULL << 17) | (1ULL << 22);
  x86_wrmsr(MSR_IA32_PERFEVTSEL1, evtsel);

  // Enable PMC1 in GLOBAL_CTRL (Bit 1)
  x86_wrmsr(MSR_IA32_PERF_GLOBAL_CTRL, (1ULL << 1));
}

typedef struct {
  uint64_t last_cycles;
  uint64_t last_instr;
} pmc_state_t;

static pmc_state_t g_pmc_state[SCHED_MAX_THREADS] = {0};

int ai_sched_arch_sample_pmc(uint32_t thread_id, ai_pmc_sample_t *out_sample) {
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

  // uint64_t instr = x86_rdpmc(1);
  uint64_t instr = 0; // Temporarily disable to avoid QEMU #GP

  uint64_t cycles_delta = cycles - g_pmc_state[thread_id].last_cycles;
  uint64_t instr_delta = 0;

  out_sample->available = 1U;
  out_sample->cycles_delta = cycles_delta;
  out_sample->instructions_delta = instr_delta;

  g_pmc_state[thread_id].last_cycles = cycles;
  g_pmc_state[thread_id].last_instr = instr;

  return 0;
}
