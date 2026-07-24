#pragma once

#include <stdint.h>

void hal_irq_enable(uint32_t irq);
void hal_irq_disable(uint32_t irq);
void hal_irq_eoi(uint32_t irq);
