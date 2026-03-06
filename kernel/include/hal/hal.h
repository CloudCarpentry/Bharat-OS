#ifndef BHARAT_HAL_H
#define BHARAT_HAL_H

/* Hardware Abstraction Layer Base Definitions */

typedef unsigned long long uint64_t;
typedef unsigned int uint32_t;
typedef unsigned short uint16_t;
typedef unsigned char uint8_t;

// CPU operations that must be implemented by each target architecture
void hal_cpu_halt(void);
void hal_cpu_enable_interrupts(void);
void hal_cpu_disable_interrupts(void);
void hal_init(void);

#endif // BHARAT_HAL_H
