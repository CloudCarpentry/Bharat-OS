/* SPDX-License-Identifier: MIT */
#ifndef BHARAT_LINUX_EPOLL_H
#define BHARAT_LINUX_EPOLL_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define LINUX_EPOLLIN        0x0001
#define LINUX_EPOLLPRI       0x0002
#define LINUX_EPOLLOUT       0x0004
#define LINUX_EPOLLERR       0x0008
#define LINUX_EPOLLHUP       0x0010
#define LINUX_EPOLLRDNORM    0x0040
#define LINUX_EPOLLRDBAND    0x0080
#define LINUX_EPOLLWRNORM    0x0100
#define LINUX_EPOLLWRBAND    0x0200
#define LINUX_EPOLLMSG       0x0400
#define LINUX_EPOLLRDHUP     0x2000
#define LINUX_EPOLLET        (1u << 31)

#define LINUX_EPOLL_CTL_ADD 1
#define LINUX_EPOLL_CTL_DEL 2
#define LINUX_EPOLL_CTL_MOD 3

#define LINUX_EPOLL_CLOEXEC  02000000

#define LINUX_EPOLL_MAX_WATCH 64

typedef union linux_epoll_data {
    void *ptr;
    int fd;
    uint32_t u32;
    uint64_t u64;
} linux_epoll_data_t;

struct linux_epoll_event {
    uint32_t events;
    linux_epoll_data_t data;
} __attribute__((packed));

typedef struct linux_epoll_watch {
    int fd;
    struct linux_epoll_event event;
    bool active;
} linux_epoll_watch_t;

typedef struct linux_epoll {
    linux_epoll_watch_t watches[LINUX_EPOLL_MAX_WATCH];
    size_t count;
    int ref_count;
    int in_use;
} linux_epoll_t;

struct bh_process;
typedef struct linux_process_state linux_process_state_t;

linux_epoll_t *linux_epoll_alloc(int flags);
void linux_epoll_ref(linux_epoll_t *ep);
void linux_epoll_unref(linux_epoll_t *ep);

long linux_epoll_ctl(linux_epoll_t *ep, int op, int fd, const struct linux_epoll_event *event);
long linux_epoll_wait(linux_epoll_t *ep, linux_process_state_t *st, struct linux_epoll_event *events, int maxevents, int timeout_ms);

#endif /* BHARAT_LINUX_EPOLL_H */
