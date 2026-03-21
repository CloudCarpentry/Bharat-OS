#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#include "hal/hal_irq.h"
#include "hal/hal.h"

// --- Mock State ---
static uint32_t mock_active_irq = 0;
static uint64_t mock_timer_vector = 32;
static uint32_t mock_eoi_called_with = 0xFFFFFFFF;
static bool mock_timer_handler_called = false;

static uint32_t mock_dispatch_called_with = 0xFFFFFFFF;
static void* mock_dispatch_ctx_received = NULL;

// --- Mock HAL Primitives ---
uint32_t hal_interrupt_get_active_irq(uint64_t hw_cause) {
    // For test purposes, we'll let the test set the expected active irq,
    // but typically hw_cause is used directly on x86/RISCV.
    return mock_active_irq;
}

uint64_t hal_irq_timer_vector(void) {
    return mock_timer_vector;
}

void hal_interrupt_end_of_interrupt(uint32_t irq) {
    mock_eoi_called_with = irq;
}

uint32_t hal_interrupt_acknowledge(void) {
    return mock_active_irq;
}

// --- Test Handlers ---
void test_timer_handler(void) {
    mock_timer_handler_called = true;
}

void test_dispatch_handler(uint32_t irq, void* ctx) {
    mock_dispatch_called_with = irq;
    mock_dispatch_ctx_received = ctx;
}

// --- Stub dependencies ---
#include "arch/arch_caps.h"

static arch_caps_t mock_caps = { .bits = 0xFFFFFFFF };

arch_caps_t arch_get_caps(void) {
    return mock_caps;
}

void spin_lock_init(void *lock) { (void)lock; }
void spin_lock(void *lock) { (void)lock; }
void spin_unlock(void *lock) { (void)lock; }
int arch_get_cpu_id(void) { return 0; }

// --- Reset Mock State ---
void reset_mocks(void) {
    mock_active_irq = 0;
    mock_timer_vector = 32;
    mock_eoi_called_with = 0xFFFFFFFF;
    mock_timer_handler_called = false;
    mock_dispatch_called_with = 0xFFFFFFFF;
    mock_dispatch_ctx_received = NULL;
}

// --- Tests ---

// 1. Test Timer IRQ Dispatch
void test_timer_irq_dispatch(void) {
    reset_mocks();

    // Simulate x86/ARM where timer vector is hit
    mock_active_irq = 32;
    mock_timer_vector = 32;

    hal_interrupt_handle_trap_irq(32 /* hw_cause */, test_timer_handler, test_dispatch_handler, NULL);

    assert(mock_timer_handler_called == true);
    assert(mock_dispatch_called_with == 0xFFFFFFFF);
    assert(mock_eoi_called_with == 32);
    printf("test_timer_irq_dispatch passed.\n");
}

// 2. Test Device IRQ Dispatch (Non-Timer)
void test_device_irq_dispatch(void) {
    reset_mocks();

    // Simulate a device interrupt (e.g. NIC on IRQ 10)
    mock_active_irq = 10;
    mock_timer_vector = 32;

    void *test_ctx = (void*)0xDEADBEEF;

    hal_interrupt_handle_trap_irq(10 /* hw_cause */, test_timer_handler, test_dispatch_handler, test_ctx);

    assert(mock_timer_handler_called == false);
    assert(mock_dispatch_called_with == 10);
    assert(mock_dispatch_ctx_received == test_ctx);
    assert(mock_eoi_called_with == 10);
    printf("test_device_irq_dispatch passed.\n");
}

// 3. Test RISC-V Timer Vector (0x8000000000000005)
void test_riscv_timer_irq_dispatch(void) {
    reset_mocks();

    // In RISC-V the timer IRQ is very large
    mock_active_irq = 5; // Cause without top bit, assuming the top bit is stripped before or HW cause is passed as such
    // Or let's just use the full vector if active_irq is 32-bit?
    // Wait, hal_interrupt_get_active_irq returns uint32_t!
    // If the timer vector is 0x8000000000000005, it cannot fit in uint32_t.
    // Let's verify how RISC-V does it!
    // Actually in RISC-V, frame->cause is 0x8000000000000005. The hal_interrupt_get_active_irq returns (uint32_t)cause.
    // That means it truncates to 5! So hal_irq_timer_vector() MUST return 5!

    // For this test, let's just test that whatever hal_irq_timer_vector returns matches the active IRQ.
    mock_active_irq = 5;
    mock_timer_vector = 5;

    hal_interrupt_handle_trap_irq(0x8000000000000005ULL, test_timer_handler, test_dispatch_handler, NULL);

    assert(mock_timer_handler_called == true);
    assert(mock_dispatch_called_with == 0xFFFFFFFF);
    assert(mock_eoi_called_with == 5);
    printf("test_riscv_timer_irq_dispatch passed.\n");
}

int main() {
    printf("Running interrupt common host tests...\n");
    hal_irq_generic_init_boot(); // Initialize the generic descriptors

    test_timer_irq_dispatch();
    test_device_irq_dispatch();
    test_riscv_timer_irq_dispatch();

    printf("All interrupt common host tests passed.\n");
    return 0;
}
