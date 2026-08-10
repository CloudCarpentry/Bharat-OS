#include <stdio.h>
#include <assert.h>

#include "hal/hal.h"
#include "hal/hal_irq.h"
#include "arch/arch_caps.h"
#include "sched/sched.h"
#include "irq/bh_irq.h"

// Basic stubs to allow linking test on host
void hal_serial_write(const char* s) { (void)s; }
void hal_timer_tick(void) {}

// Stubs for arch dependent parts
arch_caps_t arch_get_caps(void) { static arch_caps_t c; return c; }
uint32_t hal_interrupt_get_active_irq(uint64_t hw_cause) { return (uint32_t)hw_cause; }
uint64_t hal_irq_timer_vector(void) { return 0; }
void hal_irq_eoi(uint32_t irq) { (void)irq; }
uint32_t hal_cpu_get_id(void) { return 0; }

void sched_wait_queue_init(wait_queue_t* queue) {
    if (queue) {
        queue->head = NULL;
        queue->tail = NULL;
    }
}
void bh_thread_yield(void) {}
uint64_t sched_get_ticks(void) { return 100; }

static int dummy_irq_fired = 0;

static bh_irq_return_t dummy_irq_handler(void* ctx) {
    (void)ctx;
    dummy_irq_fired = 1;
    return BH_IRQ_HANDLED;
}

void hal_irq_generic_init_boot(void);

int main(void) {
    printf("[TEST] Running HAL Interrupt Common Tests...\n");

    hal_irq_generic_init_boot();

    int dev_id_dummy = 0;

    // Test Registration
    int res = hal_interrupt_register(10, (hal_irq_handler_t)dummy_irq_handler, NULL, 0, "dummy", &dev_id_dummy);
    assert(res == 0);

    assert(hal_interrupt_is_registered(10) == 1);

    // Test Dispatch
    hal_interrupt_dispatch(10);
    assert(dummy_irq_fired == 1);
    assert(hal_interrupt_get_dispatch_count(10) == 1U);

    // Test Invalid Registration
    res = hal_interrupt_register(300, (hal_irq_handler_t)dummy_irq_handler, NULL, 0, "dummy2", &dev_id_dummy);
    assert(res == -1);

    // Test Unregister
    res = hal_interrupt_unregister(10, &dev_id_dummy);
    assert(res == 0);
    assert(hal_interrupt_is_registered(10) == 0);
    assert(hal_interrupt_get_dispatch_count(10) == 1U);

    dummy_irq_fired = 0;
    hal_interrupt_dispatch(10); // Should not fire
    assert(dummy_irq_fired == 0);

    printf("[TEST] HAL Interrupt Common Tests Passed.\n");
    return 0;
}
