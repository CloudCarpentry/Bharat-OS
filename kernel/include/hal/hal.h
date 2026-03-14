#ifndef BHARAT_HAL_H
#define BHARAT_HAL_H

#include <stdint.h>


/* Hardware Abstraction Layer Base Definitions */

// CPU operations that must be implemented by each target architecture
void hal_cpu_halt(void);
void hal_cpu_reboot(void);
void hal_cpu_dump_state(void);
void hal_cpu_enable_interrupts(void);
void hal_cpu_disable_interrupts(void);
void hal_init(void);

// Optional fast-path IPI payload for small URPC messages
void hal_send_ipi_payload(uint32_t target_core, uint64_t payload);

// Early serial console support (for QEMU/OpenSBI bring-up logs)
void hal_serial_init(void);
void hal_serial_write_char(char c);
void hal_serial_write(const char *s);
void hal_serial_write_hex(uint64_t val);
int hal_serial_read_char(void);

// Interrupt and timer controller
int hal_interrupt_controller_init(void);
int hal_interrupt_route(uint32_t irq, uint32_t target_core);
uint32_t hal_interrupt_acknowledge(void);
void hal_interrupt_end_of_interrupt(uint32_t irq);
int hal_timer_source_init(uint32_t tick_hz);

// TLB Coherency
void hal_tlb_flush(unsigned long long vaddr);

// Get logical CPU ID
uint32_t hal_cpu_get_id(void);

// Core notification abstraction
void hal_core_notify(uint32_t target_core, uint64_t payload_or_reason);
void hal_core_wait_event(void);
void hal_core_poll_event(void);

#endif // BHARAT_HAL_H
