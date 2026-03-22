#include <stdio.h>
#include <assert.h>

#include "hal/hal.h"
#include "hal/hal_irq.h"
#include "arch/arch_caps.h"

// Basic stubs to allow linking test on host
void hal_serial_write(const char* s) { (void)s; }
void hal_timer_tick(void) {}

// Stubs for arch dependent parts
arch_caps_t arch_get_caps(void) { static arch_caps_t c; return c; }
uint32_t hal_interrupt_get_active_irq(uint64_t hw_cause) { return (uint32_t)hw_cause; }
uint64_t hal_irq_timer_vector(void) { return 0; }
void hal_interrupt_end_of_interrupt(uint32_t irq) { (void)irq; }


static int dummy_irq_fired = 0;

static irq_return_t dummy_irq_handler(void* ctx) {
    (void)ctx;
    dummy_irq_fired = 1;
    return IRQ_HANDLED;
}

int main(void) {
    printf("[TEST] Running HAL Interrupt Common Tests...\n");

    // Test Registration
    int res = hal_interrupt_register(10, dummy_irq_handler, NULL, 0, "dummy", NULL);
    assert(res == 0);

    assert(hal_interrupt_is_registered(10) == 1);

    // Test Dispatch
    hal_interrupt_dispatch(10);
    assert(dummy_irq_fired == 1);
    assert(hal_interrupt_get_dispatch_count(10) == 1U);

    // Test Invalid Registration
    res = hal_interrupt_register(300, dummy_irq_handler, NULL, 0, "dummy2", NULL);
    assert(res == -1);

    // Test Unregister
    res = hal_interrupt_unregister(10, NULL);
    assert(res == 0);
    assert(hal_interrupt_is_registered(10) == 0);
    assert(hal_interrupt_get_dispatch_count(10) == 0U);

    dummy_irq_fired = 0;
    hal_interrupt_dispatch(10); // Should not fire
    assert(dummy_irq_fired == 0);

    printf("[TEST] HAL Interrupt Common Tests Passed.\n");
    return 0;
}
