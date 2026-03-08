#ifndef BHARAT_HAL_INTERRUPT_H
#define BHARAT_HAL_INTERRUPT_H

#include <stdint.h>

typedef void (*hal_irq_handler_t)(void* ctx);

int hal_interrupt_register(uint32_t irq, hal_irq_handler_t handler, void* ctx);
int hal_interrupt_unregister(uint32_t irq);
void hal_interrupt_dispatch(uint32_t irq);
uint64_t hal_interrupt_get_dispatch_count(uint32_t irq);
int hal_interrupt_is_registered(uint32_t irq);

#endif // BHARAT_HAL_INTERRUPT_H
