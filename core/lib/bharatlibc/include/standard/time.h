#ifndef BHARATLIBC_TIME_H
#define BHARATLIBC_TIME_H

#include <standard/stdint.h>

typedef int32_t clockid_t;
typedef int64_t time_t;

struct timespec {
  time_t tv_sec;
  int64_t tv_nsec;
};

#define CLOCK_REALTIME 0
#define CLOCK_MONOTONIC 1

int clock_gettime(clockid_t clock_id, struct timespec *time_value);
int nanosleep(const struct timespec *request, struct timespec *remaining);

#endif /* BHARATLIBC_TIME_H */
