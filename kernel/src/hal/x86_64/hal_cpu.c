#include "hal/hal.h"
#include <stdint.h>

void hal_cpu_halt(void) {
    while (1) {
        __asm__ volatile ("hlt");
    }
}

void hal_cpu_dump_state(void) {
    // Stub
}

void hal_cpu_disable_interrupts(void) {
    __asm__ volatile ("cli");
}

void hal_cpu_enable_interrupts(void) {
    __asm__ volatile ("sti");
}

uint32_t hal_cpu_get_id(void) {
    return 0; // Stub, return 0 for now
}

void hal_serial_init(void) {
    // Stub
}

void hal_serial_write(const char *s) {
    (void)s;
}

void hal_serial_write_hex(uint64_t val) {
    (void)val;
}

void hal_init(void) {
    // Stub
}

int hal_interrupt_controller_init(void) {
    return 0;
}

void hal_tlb_flush(unsigned long long vaddr) {
    (void)vaddr;
}

void hal_send_ipi_payload(uint32_t target_cpu, uint64_t payload) {
    (void)target_cpu;
    (void)payload;
}

void hal_secure_boot_arch_check(void) {
    // Stub
}

int hal_timer_source_init(uint32_t tick_hz) {
    (void)tick_hz;
    return 0;
}

void default_timer_isr(void) {
    // Stub
}
