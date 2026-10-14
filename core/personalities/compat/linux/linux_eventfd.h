/* SPDX-License-Identifier: MIT */
#ifndef BHARAT_LINUX_EVENTFD_H
#define BHARAT_LINUX_EVENTFD_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define LINUX_EFD_SEMAPHORE (1 << 0)
#define LINUX_EFD_CLOEXEC   02000000
#define LINUX_EFD_NONBLOCK  00004000

typedef struct linux_eventfd {
    uint64_t counter;
    int flags;
    int ref_count;
    int in_use;
} linux_eventfd_t;

linux_eventfd_t *linux_eventfd_alloc(unsigned int initval, int flags);
void linux_eventfd_ref(linux_eventfd_t *efd);
void linux_eventfd_unref(linux_eventfd_t *efd);

long linux_eventfd_read(linux_eventfd_t *efd, void *buf, size_t count, bool nonblock);
long linux_eventfd_write(linux_eventfd_t *efd, const void *buf, size_t count, bool nonblock);
bool linux_eventfd_is_readable(linux_eventfd_t *efd);
bool linux_eventfd_is_writable(linux_eventfd_t *efd);

#endif /* BHARAT_LINUX_EVENTFD_H */
