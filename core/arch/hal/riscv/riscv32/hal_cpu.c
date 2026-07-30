#include "sched/ai_sched.h"
#include "hal/hal.h"
#include "hal/hal_timer.h"
#include "hal/riscv_bsp.h"
#include "secure_boot.h"

#include "../../arch/riscv/boot/sbi.h"
#include "trap.h"

// RISC-V 32 HAL Implementation

static uint64_t g_boot_hart_id;
static uint64_t g_boot_fdt_ptr;

int hal_secure_boot_arch_check(const bharat_boot_policy_t *policy) {
  if (!policy) {
    return -1;
  }
  if (policy->security_level == BHARAT_BOOT_SECURITY_ENFORCED &&
      g_boot_fdt_ptr == 0U) {
    return -2;
  }
  return 0;
}

void hal_core_notify(uint32_t target_core, uint64_t payload_or_reason) {
  hal_send_ipi_payload(target_core, payload_or_reason);
}

void hal_core_wait_event(void) {
  __asm__ volatile("wfi" : : : "memory");
}

void hal_core_poll_event(void) {
}

void hal_riscv_set_boot_info(uint64_t hart_id, uint64_t fdt_ptr) {
  g_boot_hart_id = hart_id;
  g_boot_fdt_ptr = fdt_ptr;
}

uint64_t hal_riscv_boot_hart_id(void) { return g_boot_hart_id; }
uint64_t hal_riscv_boot_fdt_ptr(void) { return g_boot_fdt_ptr; }

static void hal_riscv_send_ipi_payload(const unsigned long *hart_mask,
                                uint64_t payload) {
  sbi_send_ipi_payload(hart_mask, payload);
}

void hal_send_ipi_payload(uint32_t target_core, uint64_t payload) {
  unsigned long hart_mask = (1UL << target_core);
  hal_riscv_send_ipi_payload(&hart_mask, payload);
}

void hal_cpu_halt(void) {
  __asm__ volatile("wfi");
}

void hal_cpu_reboot(void) {
  sbi_system_reset(0, 0);
  while (1) {
    __asm__ volatile("wfi");
  }
}

bool hal_cpu_is_syscall(const void *trap_frame) {
    if (!trap_frame) return false;
    const trap_frame_t *tf = (const trap_frame_t *)trap_frame;
    return (tf->cause == 8);
}

bool hal_cpu_is_page_fault(const void *trap_frame) {
    if (!trap_frame) return false;
    const trap_frame_t *tf = (const trap_frame_t *)trap_frame;
    return (tf->cause == 12 || tf->cause == 13 || tf->cause == 15);
}

bool hal_cpu_is_access_fault(const void *trap_frame) {
    if (!trap_frame) return false;
    const trap_frame_t *tf = (const trap_frame_t *)trap_frame;
    return (tf->cause == 1 || tf->cause == 5 || tf->cause == 7);
}

bool hal_cpu_is_fp_simd_fault(const void *trap_frame) {
    if (!trap_frame) return false;
    const trap_frame_t *tf = (const trap_frame_t *)trap_frame;
    if (tf->cause == 2) {
        uint32_t sstatus;
        __asm__ volatile("csrr %0, sstatus" : "=r"(sstatus));
        return ((sstatus & (3U << 13)) == 0);
    }
    return false;
}

bool hal_cpu_is_illegal_instruction(const void *trap_frame) {
    if (!trap_frame) return false;
    const trap_frame_t *tf = (const trap_frame_t *)trap_frame;
    return (tf->cause == 2);
}

uint32_t hal_interrupt_get_active_irq(uint64_t hw_cause) {
    return (uint32_t)(hw_cause & ~(1ULL << 31));
}

uint64_t hal_irq_timer_vector(void) {
    return 5U;
}

uint64_t hal_cpu_get_fault_address(const void *trap_frame) {
    (void)trap_frame;
    uint32_t stval;
    __asm__ volatile("csrr %0, stval" : "=r"(stval));
    return stval;
}

__attribute__((weak)) void hal_cpu_dump_trap_frame(const void *trap_frame) {
  if (!trap_frame) return;
  const trap_frame_t *tf = (const trap_frame_t *)trap_frame;
  hal_serial_write("\n--- RISC-V 32 Trap Frame Dump ---\n");
  hal_serial_write("SCAUSE: "); hal_serial_write_hex(tf->cause); hal_serial_write("\n");
  hal_serial_write("SEPC:   "); hal_serial_write_hex(tf->pc); hal_serial_write("\n");
  hal_serial_write("SP:     "); hal_serial_write_hex(tf->sp); hal_serial_write("\n");
}

void hal_cpu_dump_state(void) {
  uint32_t sepc, scause, stval, sstatus, fp, sp;
  __asm__ volatile("csrr %0, sepc" : "=r"(sepc));
  __asm__ volatile("csrr %0, scause" : "=r"(scause));
  __asm__ volatile("csrr %0, stval" : "=r"(stval));
  __asm__ volatile("csrr %0, sstatus" : "=r"(sstatus));
  __asm__ volatile("mv %0, s0" : "=r"(fp));
  __asm__ volatile("mv %0, sp" : "=r"(sp));

  hal_serial_write("\n--- RISC-V 32 CPU State Dump ---\n");
  hal_serial_write("SEPC: "); hal_serial_write_hex(sepc); hal_serial_write("\n");
  hal_serial_write("SCAUSE: "); hal_serial_write_hex(scause); hal_serial_write("\n");
}

void hal_cpu_enable_interrupts(void) {
  __asm__ volatile("csrci sstatus, 2");
}

void hal_cpu_disable_interrupts(void) {
  __asm__ volatile("csrci sstatus, 2");
}

extern void trap_entry(void);
extern void arch_discover_hw_caps(void);

void hal_init(void) {
  arch_discover_hw_caps();
  __asm__ volatile("csrw stvec, %0" : : "r"((uintptr_t)trap_entry));
  __asm__ volatile("csrw sscratch, 0");
  hal_serial_init();
}

void hal_tlb_flush(unsigned long long vaddr) {
  uintptr_t va = (uintptr_t)vaddr;
  __asm__ volatile("sfence.vma %0, x0" ::"r"(va) : "memory");
}

uint32_t hal_cpu_get_id(void) {
  return (uint32_t)g_boot_hart_id;
}

#define SCHED_MAX_THREADS 64U

typedef struct {
  uint64_t last_cycles;
  uint64_t last_instr;
} pmc_state_t;

static pmc_state_t g_pmc_state[SCHED_MAX_THREADS] = {0};

int ai_sched_arch_sample_pmc(uint32_t thread_id, ai_pmc_sample_t *out_sample) {
  if (!out_sample || thread_id >= SCHED_MAX_THREADS) return -1;
  uint32_t cycles = 0, instr = 0;
  __asm__ volatile("rdcycle %0" : "=r"(cycles));
  __asm__ volatile("rdinstret %0" : "=r"(instr));
  out_sample->available = 1U;
  out_sample->cycles_delta = cycles - (uint32_t)g_pmc_state[thread_id].last_cycles;
  out_sample->instructions_delta = instr - (uint32_t)g_pmc_state[thread_id].last_instr;
  g_pmc_state[thread_id].last_cycles = cycles;
  g_pmc_state[thread_id].last_instr = instr;
  return 0;
}
