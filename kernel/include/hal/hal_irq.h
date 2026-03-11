#ifndef BHARAT_HAL_IRQ_H
#define BHARAT_HAL_IRQ_H

#include <stdint.h>

// Forward definition
typedef void (*hal_irq_handler_t)(void* ctx);

// Boot Core Init
int hal_irq_init_boot(void);

// Per-core Init
int hal_irq_init_cpu_local(void);

// Enable / Disable specific vector
int hal_irq_enable(uint32_t vector);
int hal_irq_disable(uint32_t vector);

// Send IPI
int hal_ipi_send(uint32_t cpu_id, uint32_t reason_vector);

// End Of Interrupt
void hal_irq_eoi(uint32_t vector);

// Register handler
int hal_interrupt_register(uint32_t irq, hal_irq_handler_t handler, void* ctx);
int hal_interrupt_unregister(uint32_t irq);
void hal_interrupt_dispatch(uint32_t irq);
uint64_t hal_interrupt_get_dispatch_count(uint32_t irq);
int hal_interrupt_is_registered(uint32_t irq);

#endif
