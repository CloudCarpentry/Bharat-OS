/* SPDX-License-Identifier: MIT */
#ifndef BHARAT_LINUX_TIMERFD_H
#define BHARAT_LINUX_TIMERFD_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "linux_types.h"

#define LINUX_TFD_TIMER_ABSTIME (1 << 0)
#define LINUX_TFD_CLOEXEC       02000000
#define LINUX_TFD_NONBLOCK      00004000

typedef struct linux_timerfd {
    int clockid;
    int flags;
    uint64_t interval_ns;
    uint64_t expiry_ns;
    int ref_count;
    int in_use;
} linux_timerfd_t;

linux_timerfd_t *linux_timerfd_alloc(int clockid, int flags);
void linux_timerfd_ref(linux_timerfd_t *tfd);
void linux_timerfd_unref(linux_timerfd_t *tfd);

long linux_timerfd_settime(linux_timerfd_t *tfd, int flags, uint64_t init_ns, uint64_t intv_ns, uint64_t *old_init_ns, uint64_t *old_intv_ns);
long linux_timerfd_gettime(linux_timerfd_t *tfd, uint64_t *cur_init_ns, uint64_t *cur_intv_ns);
long linux_timerfd_read(linux_timerfd_t *tfd, void *buf, size_t count, bool nonblock);
bool linux_timerfd_is_readable(linux_timerfd_t *tfd);

#endif /* BHARAT_LINUX_TIMERFD_H */
