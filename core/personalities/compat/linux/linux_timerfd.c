/* SPDX-License-Identifier: MIT */
#include "linux_timerfd.h"
#include "linux_errno.h"
#include "syscall/usercopy.h"
#include "sched/sched.h"
#include "time/ktime.h"
#include "lib/base/string.h"

#define MAX_LINUX_TIMERFDS 64

static linux_timerfd_t g_linux_timerfd_pool[MAX_LINUX_TIMERFDS];

linux_timerfd_t *linux_timerfd_alloc(int clockid, int flags) {
    for (int i = 0; i < MAX_LINUX_TIMERFDS; i++) {
        if (!g_linux_timerfd_pool[i].in_use) {
            memset(&g_linux_timerfd_pool[i], 0, sizeof(linux_timerfd_t));
            g_linux_timerfd_pool[i].in_use = 1;
            g_linux_timerfd_pool[i].clockid = clockid;
            g_linux_timerfd_pool[i].flags = flags;
            g_linux_timerfd_pool[i].ref_count = 1;
            return &g_linux_timerfd_pool[i];
        }
    }
    return NULL;
}

void linux_timerfd_ref(linux_timerfd_t *tfd) {
    if (tfd) {
        tfd->ref_count++;
    }
}

void linux_timerfd_unref(linux_timerfd_t *tfd) {
    if (!tfd) return;
    tfd->ref_count--;
    if (tfd->ref_count <= 0) {
        tfd->in_use = 0;
        tfd->interval_ns = 0;
        tfd->expiry_ns = 0;
    }
}

long linux_timerfd_settime(linux_timerfd_t *tfd, int flags, uint64_t init_ns, uint64_t intv_ns, uint64_t *old_init_ns, uint64_t *old_intv_ns) {
    if (!tfd) return -LINUX_EINVAL;

    uint64_t now = bh_ktime_now();

    if (old_init_ns && old_intv_ns) {
        *old_intv_ns = tfd->interval_ns;
        if (tfd->expiry_ns > now) {
            *old_init_ns = tfd->expiry_ns - now;
        } else {
            *old_init_ns = 0;
        }
    }

    tfd->interval_ns = intv_ns;
    if (init_ns == 0) {
        tfd->expiry_ns = 0; // Disarm
    } else if (flags & LINUX_TFD_TIMER_ABSTIME) {
        tfd->expiry_ns = init_ns;
    } else {
        tfd->expiry_ns = now + init_ns;
    }

    return 0;
}

long linux_timerfd_gettime(linux_timerfd_t *tfd, uint64_t *cur_init_ns, uint64_t *cur_intv_ns) {
    if (!tfd || !cur_init_ns || !cur_intv_ns) return -LINUX_EINVAL;

    uint64_t now = bh_ktime_now();
    *cur_intv_ns = tfd->interval_ns;
    if (tfd->expiry_ns > now) {
        *cur_init_ns = tfd->expiry_ns - now;
    } else {
        *cur_init_ns = 0;
    }
    return 0;
}

bool linux_timerfd_is_readable(linux_timerfd_t *tfd) {
    if (!tfd || tfd->expiry_ns == 0) return false;
    return bh_ktime_now() >= tfd->expiry_ns;
}

long linux_timerfd_read(linux_timerfd_t *tfd, void *buf, size_t count, bool nonblock) {
    if (!tfd || !buf) return -LINUX_EFAULT;
    if (count < sizeof(uint64_t)) return -LINUX_EINVAL;
    if (tfd->expiry_ns == 0) return -LINUX_EINVAL; // Disarmed

    uint64_t now = bh_ktime_now();
    while (now < tfd->expiry_ns) {
        if (nonblock || (tfd->flags & LINUX_TFD_NONBLOCK)) {
            return -LINUX_EAGAIN;
        }
        bh_thread_yield();
        now = bh_ktime_now();
    }

    uint64_t expirations = 1;
    if (tfd->interval_ns > 0) {
        uint64_t elapsed = now - tfd->expiry_ns;
        expirations += elapsed / tfd->interval_ns;
        tfd->expiry_ns += expirations * tfd->interval_ns;
    } else {
        tfd->expiry_ns = 0; // One-shot disarms after firing
    }

    bh_status_t s = bh_copy_to_user(buf, &expirations, sizeof(uint64_t));
    if (s != BH_OK) {
        return -LINUX_EFAULT;
    }

    return (long)sizeof(uint64_t);
}
