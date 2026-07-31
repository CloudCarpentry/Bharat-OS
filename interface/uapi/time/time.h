#ifndef BHARAT_UAPI_TIME_H
#define BHARAT_UAPI_TIME_H

#include <stdint.h>

/**
 * @brief User-space time type representing monotonic nanoseconds.
 */
typedef uint64_t bh_time_t;
typedef uint64_t bh_deadline_t;

#define BH_CLOCK_REALTIME   0
#define BH_CLOCK_MONOTONIC  1

#define BH_NS_PER_MS UINT64_C(1000000)

#endif /* BHARAT_UAPI_TIME_H */
