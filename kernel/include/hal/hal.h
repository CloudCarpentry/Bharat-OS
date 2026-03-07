#ifndef BHARAT_HAL_H
#define BHARAT_HAL_H

#include <stdint.h>

#include "interrupt.h"
#include "timer.h"

/* Hardware Abstraction Layer Base Definitions */

// CPU operations that must be implemented by each target architecture
void hal_cpu_halt(void);
void hal_cpu_enable_interrupts(void);
void hal_cpu_disable_interrupts(void);
void hal_init(void);

// Optional fast-path IPI payload for small URPC messages
void hal_send_ipi_payload(uint32_t target_core, uint64_t payload);

// Early serial console support (for QEMU/OpenSBI bring-up logs)
void hal_serial_init(void);
void hal_serial_write_char(char c);
void hal_serial_write(const char* s);
int hal_serial_read_char(void);

// Interrupt and timer controller
int hal_interrupt_controller_init(void);
int hal_interrupt_route(uint32_t irq, uint32_t target_core);
int hal_timer_source_init(uint32_t tick_hz);

// TLB Coherency
void hal_tlb_flush(unsigned long long vaddr);

#endif // BHARAT_HAL_H
