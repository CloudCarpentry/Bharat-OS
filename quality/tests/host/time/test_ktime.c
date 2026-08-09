#include <stdio.h>
#include <assert.h>
#include "time/ktime.h"
#include "hal/hal_timer.h"

// Mock HAL state
static uint64_t mock_timer_counter = 0;
static uint64_t mock_timer_freq = 1000000000; // 1 GHz
static uint64_t mock_programmed_oneshot = 0;

uint64_t hal_timer_read_counter(void) {
    return mock_timer_counter;
}

uint64_t hal_timer_read_freq(void) {
    return mock_timer_freq;
}

bool hal_timer_is_per_cpu(void) {
    return true;
}

void hal_timer_program_oneshot(uint64_t ns) {
    uint64_t freq = hal_timer_read_freq();
    uint64_t ticks;
    if (hal_timer_ns_to_ticks_ceil(ns, freq, &ticks)) {
        mock_programmed_oneshot = ticks;
    }
}

// Ignore panic in tests
void kernel_panic(const char *msg) {
    (void)msg;
}

static void test_monotonicity(void) {
    mock_timer_counter = 1000;
    bh_ktime_t t1 = bh_ktime_now();
    mock_timer_counter = 2000;
    bh_ktime_t t2 = bh_ktime_now();
    assert(t2 > t1);
    printf("test_monotonicity passed\n");
}

static void test_zero_duration(void) {
    mock_timer_counter = 5000;
    bh_ktime_t now = bh_ktime_now();
    bh_kdeadline_t deadline = bh_deadline_after_ns(0);
    assert(deadline == now);
    assert(bh_deadline_expired(deadline));
    printf("test_zero_duration passed\n");
}

static void test_normal_deadline(void) {
    mock_timer_counter = 10000; // 10 us at 1 GHz
    bh_kdeadline_t deadline = bh_deadline_after_ns(5000); // + 5 us
    assert(!bh_deadline_expired(deadline));

    mock_timer_counter = 14999;
    assert(!bh_deadline_expired(deadline));

    mock_timer_counter = 15000;
    assert(bh_deadline_expired(deadline));
    printf("test_normal_deadline passed\n");
}

static void test_overflow_saturation(void) {
    mock_timer_counter = 1000; // Time is small
    bh_kdeadline_t deadline = bh_deadline_after_ns(UINT64_MAX); // Should saturate to infinity
    assert(deadline == BH_KDEADLINE_INFINITE);
    assert(!bh_deadline_expired(deadline));

    // Simulate current time near MAX
    mock_timer_freq = 1000000000;
    mock_timer_counter = UINT64_MAX - 1000;
    bh_kdeadline_t deadline2 = bh_deadline_after_ns(2000);
    assert(deadline2 == BH_KDEADLINE_INFINITE);
    assert(!bh_deadline_expired(deadline2));

    // Reset mock
    mock_timer_freq = 1000000000;
    printf("test_overflow_saturation passed\n");
}

static void test_remaining_time(void) {
    mock_timer_counter = 10000;
    bh_kdeadline_t deadline = bh_deadline_after_ns(5000); // Absolute deadline = 15000

    mock_timer_counter = 12000;
    uint64_t remaining = bh_deadline_remaining_ns(deadline);
    assert(remaining == 3000);

    mock_timer_counter = 16000; // Expired
    remaining = bh_deadline_remaining_ns(deadline);
    assert(remaining == 0);
    printf("test_remaining_time passed\n");
}

static void test_counter_conversion(void) {
    mock_timer_freq = 24000000; // 24 MHz
    mock_timer_counter = 48000000; // 2 seconds
    bh_ktime_t t1 = bh_ktime_now();
    assert(t1 == 2000000000ULL);

    mock_timer_counter = 48000012; // 2 seconds + 12 ticks
    bh_ktime_t t2 = bh_ktime_now();
    assert(t2 == 2000000500ULL); // 12 ticks at 24MHz is 500ns
    printf("test_counter_conversion passed\n");
}

static void test_fallback_logic(void) {
    mock_timer_freq = 1000000000;
    mock_timer_counter = 50000; // 50 us

    bh_kdeadline_t deadline = bh_deadline_after_ns(20000); // 70 us
    bool result = hal_timer_program_deadline_ns(deadline);
    assert(result == true);
    assert(mock_programmed_oneshot == 20000); // 1 GHz = 1 tick per ns

    mock_timer_counter = 80000; // 80 us (already expired)
    result = hal_timer_program_deadline_ns(deadline);
    assert(result == false);

    mock_timer_freq = 24000000; // 24 MHz
    mock_timer_counter = 0; // 0 ns
    bh_kdeadline_t deadline2 = bh_deadline_after_ns(50); // 50 ns * 24MHz = 1.2 ticks -> 2
    hal_timer_program_deadline_ns(deadline2);
    assert(mock_programmed_oneshot == 2);

    bh_kdeadline_t deadline3 = bh_deadline_after_ns(1); // 1 ns * 24MHz = 0.024 ticks -> 1
    hal_timer_program_deadline_ns(deadline3);
    assert(mock_programmed_oneshot == 1);

    printf("test_fallback_logic passed\n");
}

int main() {
    test_monotonicity();
    test_zero_duration();
    test_normal_deadline();
    test_overflow_saturation();
    test_remaining_time();
    test_counter_conversion();
    test_fallback_logic();
    printf("All ktime tests passed.\n");
    return 0;
}
void sched_on_timer_tick(void) {}
void hal_timer_arch_get_caps(hal_timer_caps_t *caps) {
    caps->has_counter = true;
    caps->has_monotonic_ns = true;
    caps->has_precise_oneshot = true;
    caps->has_native_absolute_deadline = false;
    caps->is_per_cpu = true;
}
