#ifndef BHARAT_HAL_H
#define BHARAT_HAL_H

#include <stdint.h>

/* Hardware Abstraction Layer Base Definitions */

// CPU operations that must be implemented by each target architecture
void hal_cpu_halt(void);
void hal_cpu_enable_interrupts(void);
void hal_cpu_disable_interrupts(void);
void hal_init(void);

// Optional fast-path IPI payload for small URPC messages
void hal_send_ipi_payload(uint32_t target_core, uint64_t payload);

// TLB Coherency
void hal_tlb_flush(unsigned long long vaddr);

#endif // BHARAT_HAL_H
