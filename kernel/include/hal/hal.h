#ifndef BHARAT_HAL_H
#define BHARAT_HAL_H

#include <stdint.h>

/* Hardware Abstraction Layer Base Definitions */

// CPU operations that must be implemented by each target architecture
void hal_cpu_halt(void);
void hal_cpu_enable_interrupts(void);
void hal_cpu_disable_interrupts(void);
void hal_init(void);

// TLB Coherency
void hal_tlb_flush(unsigned long long vaddr);

#endif // BHARAT_HAL_H
