#ifndef BHARAT_KERNEL_KTIME_H
#define BHARAT_KERNEL_KTIME_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Kernel-internal representation of monotonic nanoseconds.
 *
 * Distinct from UAPI bh_time_t/bh_deadline_t to avoid leaking
 * internal implementation details directly to the UAPI ABI.
 */
typedef uint64_t bh_ktime_t;
typedef uint64_t bh_kdeadline_t;

#define BH_KTIME_NS_PER_MS      UINT64_C(1000000)
#define BH_KTIME_NS_PER_SEC     UINT64_C(1000000000)
#define BH_KDEADLINE_INFINITE   UINT64_MAX

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Returns the current monotonic time in nanoseconds.
 * @return Monotonic time in nanoseconds.
 */
bh_ktime_t bh_ktime_now(void);

/**
 * @brief Calculates a safe, absolute deadline from a relative duration.
 *
 * Safe against overflow. Overflows saturate to BH_KDEADLINE_INFINITE.
 *
 * @param duration_ns The relative duration in nanoseconds.
 * @return The absolute deadline in monotonic nanoseconds.
 */
bh_kdeadline_t bh_deadline_after_ns(uint64_t duration_ns);

/**
 * @brief Checks if a given absolute deadline has expired.
 *
 * @param deadline The absolute deadline to check.
 * @return true if expired (current time >= deadline), false otherwise.
 */
bool bh_deadline_expired(bh_kdeadline_t deadline);

/**
 * @brief Calculates the remaining time before a deadline expires.
 *
 * @param deadline The absolute deadline.
 * @return The remaining duration in nanoseconds, or 0 if expired.
 */
uint64_t bh_deadline_remaining_ns(bh_kdeadline_t deadline);

#ifdef __cplusplus
}
#endif

#endif // BHARAT_KERNEL_KTIME_H
