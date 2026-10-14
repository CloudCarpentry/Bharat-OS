/* SPDX-License-Identifier: MIT */
#include "linux_eventfd.h"
#include "linux_errno.h"
#include "syscall/usercopy.h"
#include "sched/sched.h"
#include "lib/base/string.h"

#define MAX_LINUX_EVENTFDS 64

static linux_eventfd_t g_linux_eventfd_pool[MAX_LINUX_EVENTFDS];

linux_eventfd_t *linux_eventfd_alloc(unsigned int initval, int flags) {
    for (int i = 0; i < MAX_LINUX_EVENTFDS; i++) {
        if (!g_linux_eventfd_pool[i].in_use) {
            memset(&g_linux_eventfd_pool[i], 0, sizeof(linux_eventfd_t));
            g_linux_eventfd_pool[i].in_use = 1;
            g_linux_eventfd_pool[i].counter = initval;
            g_linux_eventfd_pool[i].flags = flags;
            g_linux_eventfd_pool[i].ref_count = 1;
            return &g_linux_eventfd_pool[i];
        }
    }
    return NULL;
}

void linux_eventfd_ref(linux_eventfd_t *efd) {
    if (efd) {
        efd->ref_count++;
    }
}

void linux_eventfd_unref(linux_eventfd_t *efd) {
    if (!efd) return;
    efd->ref_count--;
    if (efd->ref_count <= 0) {
        efd->in_use = 0;
        efd->counter = 0;
    }
}

bool linux_eventfd_is_readable(linux_eventfd_t *efd) {
    if (!efd) return false;
    return efd->counter > 0;
}

bool linux_eventfd_is_writable(linux_eventfd_t *efd) {
    if (!efd) return false;
    return efd->counter < 0xFFFFFFFFFFFFFFFEULL;
}

long linux_eventfd_read(linux_eventfd_t *efd, void *buf, size_t count, bool nonblock) {
    if (!efd || !buf) return -LINUX_EFAULT;
    if (count < sizeof(uint64_t)) return -LINUX_EINVAL;

    while (efd->counter == 0) {
        if (nonblock || (efd->flags & LINUX_EFD_NONBLOCK)) {
            return -LINUX_EAGAIN;
        }
        bh_thread_yield();
    }

    uint64_t val = 0;
    if (efd->flags & LINUX_EFD_SEMAPHORE) {
        val = 1;
        efd->counter -= 1;
    } else {
        val = efd->counter;
        efd->counter = 0;
    }

    bh_status_t s = bh_copy_to_user(buf, &val, sizeof(uint64_t));
    if (s != BH_OK) {
        return -LINUX_EFAULT;
    }

    return (long)sizeof(uint64_t);
}

long linux_eventfd_write(linux_eventfd_t *efd, const void *buf, size_t count, bool nonblock) {
    if (!efd || !buf) return -LINUX_EFAULT;
    if (count < sizeof(uint64_t)) return -LINUX_EINVAL;

    uint64_t val = 0;
    bh_status_t s = bh_copy_from_user(&val, buf, sizeof(uint64_t));
    if (s != BH_OK) {
        return -LINUX_EFAULT;
    }

    if (val == 0xFFFFFFFFFFFFFFFFULL) {
        return -LINUX_EINVAL;
    }

    while (0xFFFFFFFFFFFFFFFEULL - efd->counter < val) {
        if (nonblock || (efd->flags & LINUX_EFD_NONBLOCK)) {
            return -LINUX_EAGAIN;
        }
        bh_thread_yield();
    }

    efd->counter += val;
    return (long)sizeof(uint64_t);
}
