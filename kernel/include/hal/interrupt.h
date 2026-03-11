#ifndef BHARAT_HAL_OLD_INTERRUPT_H
#define BHARAT_HAL_OLD_INTERRUPT_H

#include "hal_irq.h"

// Transitional wrapper for old hal.h
int hal_interrupt_controller_init(void);
int hal_interrupt_route(uint32_t irq, uint32_t target_core);
uint32_t hal_interrupt_acknowledge(void);
void hal_interrupt_end_of_interrupt(uint32_t irq);

#endif // BHARAT_HAL_OLD_INTERRUPT_H
