#include "hal/hal.h"
#include <stdint.h>

void hal_cpu_init(void) {}

bool hal_cpu_is_page_fault(const void *trap_frame) {
    (void)trap_frame;
    return false;
}

bool hal_cpu_is_access_fault(const void *trap_frame) {
    (void)trap_frame;
    return false;
}

bool hal_cpu_is_fp_simd_fault(const void *trap_frame) {
    (void)trap_frame;
    return false;
}

bool hal_cpu_is_illegal_instruction(const void *trap_frame) {
    (void)trap_frame;
    return false;
}

uint32_t hal_interrupt_get_active_irq(uint64_t hw_cause) {
    (void)hw_cause;
    return hal_interrupt_acknowledge();
}

uint64_t hal_irq_timer_vector(void) {
    return 30U; // Generic ARM timer vector
}

uint64_t hal_cpu_get_fault_address(const void *trap_frame) {
    (void)trap_frame;
    return 0;
}

void hal_init(void) {}
uint32_t hal_mm_backend_caps(void) { return 0; }
void hal_send_ipi_payload(uint32_t cpu, uint64_t payload) { (void)cpu; (void)payload; }
void hal_irq_init_boot(void) {}
uint32_t hal_interrupt_acknowledge(void) { return 0; }

void hal_cpu_halt(void) { while(1); }
void hal_interrupt_end_of_interrupt(uint32_t irq) { (void)irq; }
void hal_cpu_enable_interrupts(void) {}
void hal_cpu_disable_interrupts(void) {}
void hal_ipi_send(uint32_t target_cpu, uint32_t vector) { (void)target_cpu; (void)vector; }
uint32_t hal_cpu_get_id(void) { return 0; }
void hal_core_poll_event(void) {}
void hal_cpu_dump_trap_frame(const void *trap_frame) { (void)trap_frame; }
bool active_mem_protect = false;
void hal_timer_isr(void) {}
void hal_cpu_dump_state(void) {}
bool hal_secure_boot_arch_check(void) { return true; }

void hal_core_notify(uint32_t target_core, uint64_t payload_or_reason) { (void)target_core; (void)payload_or_reason; }
uint32_t hal_get_cpu_id(void) { return 0; }
void hal_mm_get_zone_limits(uint32_t zone, uintptr_t *start, uintptr_t *end) {
    (void)zone; 
    if (start) *start = 0;
    if (end) *end = 0;
}

void *__aeabi_read_tp(void) {
    return 0;
}
