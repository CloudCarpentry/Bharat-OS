#include "time/ktime.h"
#include "hal/hal_timer.h"
#include "panic.h"

bh_ktime_t bh_ktime_now(void) {
    uint64_t now_ns;
    if (!hal_timer_monotonic_ns(&now_ns)) {
        kernel_panic("KTIME: Monotonic nanosecond clock is unavailable or uninitialized!");
    }
    return now_ns;
}

bh_kdeadline_t bh_deadline_after_ns(uint64_t duration_ns) {
    if (duration_ns == BH_KDEADLINE_INFINITE) {
        return BH_KDEADLINE_INFINITE;
    }

    bh_ktime_t now = bh_ktime_now();

    // Overflow check
    if (UINT64_MAX - now < duration_ns) {
        return BH_KDEADLINE_INFINITE;
    }

    return now + duration_ns;
}

bool bh_deadline_expired(bh_kdeadline_t deadline) {
    if (deadline == BH_KDEADLINE_INFINITE) {
        return false;
    }

    return bh_ktime_now() >= deadline;
}

uint64_t bh_deadline_remaining_ns(bh_kdeadline_t deadline) {
    if (deadline == BH_KDEADLINE_INFINITE) {
        return BH_KDEADLINE_INFINITE;
    }

    bh_ktime_t now = bh_ktime_now();
    if (now >= deadline) {
        return 0;
    }

    return deadline - now;
}
