#include <stdio.h>
#include <assert.h>

#include "hal/hal.h"
#include "hal/timer.h"
#include "sched.h"

//
// Mocks for Architecture-Specific / External Dependencies
//

static uint32_t mock_hal_timer_source_init_called_with_hz = 0;
static int mock_hal_timer_source_init_return_val = 0;
static int mock_sched_on_timer_tick_called = 0;

int hal_timer_source_init(uint32_t tick_hz) {
    mock_hal_timer_source_init_called_with_hz = tick_hz;
    return mock_hal_timer_source_init_return_val;
}

void sched_on_timer_tick(void) {
    mock_sched_on_timer_tick_called++;
}

static void reset_mocks(void) {
    mock_hal_timer_source_init_called_with_hz = 0;
    mock_hal_timer_source_init_return_val = 0;
    mock_sched_on_timer_tick_called = 0;
}

//
// Test Cases
//

static void test_hal_timer_init(void) {
    printf("  Testing hal_timer_init...\n");
    reset_mocks();

    // Test zero hz
    int res = hal_timer_init(0);
    assert(res == -1);
    assert(mock_hal_timer_source_init_called_with_hz == 0);

    // Test valid hz
    mock_hal_timer_source_init_return_val = 0;
    res = hal_timer_init(100);
    assert(res == 0);
    assert(mock_hal_timer_source_init_called_with_hz == 100);

    // After init, monotonic ticks should be 0
    assert(hal_timer_monotonic_ticks() == 0);
}

static void test_hal_timer_set_periodic(void) {
    printf("  Testing hal_timer_set_periodic...\n");
    reset_mocks();

    // Setup initial state just to be safe
    hal_timer_init(100);

    // Test zero hz
    int res = hal_timer_set_periodic(0);
    assert(res == -1);

    // Test valid hz
    mock_hal_timer_source_init_return_val = 0;
    res = hal_timer_set_periodic(250);
    assert(res == 0);
    assert(mock_hal_timer_source_init_called_with_hz == 250);
}

static void test_hal_timer_tick_and_monotonic(void) {
    printf("  Testing hal_timer_tick & monotonic ticks...\n");
    reset_mocks();

    // Initialize to reset tick count
    hal_timer_init(100);
    assert(hal_timer_monotonic_ticks() == 0);

    // Tick once
    hal_timer_tick();
    assert(hal_timer_monotonic_ticks() == 1);
    assert(mock_sched_on_timer_tick_called == 1);

    // Tick again
    hal_timer_tick();
    assert(hal_timer_monotonic_ticks() == 2);
    assert(mock_sched_on_timer_tick_called == 2);
}

int main(void) {
    printf("[TEST] Running HAL Timer Common Tests...\n");

    test_hal_timer_init();
    test_hal_timer_set_periodic();
    test_hal_timer_tick_and_monotonic();

    printf("[TEST] All HAL Timer Common Tests Passed.\n");
    return 0;
}
