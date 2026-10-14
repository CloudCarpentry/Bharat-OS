/* SPDX-License-Identifier: MIT */
#include "linux_epoll.h"
#include "linux_compat.h"
#include "linux_errno.h"
#include "linux_pipe.h"
#include "linux_eventfd.h"
#include "linux_timerfd.h"
#include "syscall/usercopy.h"
#include "sched/sched.h"
#include "time/ktime.h"
#include "lib/base/string.h"

#define MAX_LINUX_EPOLLS 64

static linux_epoll_t g_linux_epoll_pool[MAX_LINUX_EPOLLS];

linux_epoll_t *linux_epoll_alloc(int flags) {
    (void)flags;
    for (int i = 0; i < MAX_LINUX_EPOLLS; i++) {
        if (!g_linux_epoll_pool[i].in_use) {
            memset(&g_linux_epoll_pool[i], 0, sizeof(linux_epoll_t));
            g_linux_epoll_pool[i].in_use = 1;
            g_linux_epoll_pool[i].ref_count = 1;
            return &g_linux_epoll_pool[i];
        }
    }
    return NULL;
}

void linux_epoll_ref(linux_epoll_t *ep) {
    if (ep) {
        ep->ref_count++;
    }
}

void linux_epoll_unref(linux_epoll_t *ep) {
    if (!ep) return;
    ep->ref_count--;
    if (ep->ref_count <= 0) {
        ep->in_use = 0;
        ep->count = 0;
    }
}

long linux_epoll_ctl(linux_epoll_t *ep, int op, int fd, const struct linux_epoll_event *event) {
    if (!ep || fd < 0 || fd >= LINUX_MAX_FDS) return -LINUX_EBADF;

    if (op == LINUX_EPOLL_CTL_ADD) {
        if (!event) return -LINUX_EFAULT;
        for (size_t i = 0; i < ep->count; i++) {
            if (ep->watches[i].active && ep->watches[i].fd == fd) {
                return -LINUX_EEXIST;
            }
        }
        if (ep->count >= LINUX_EPOLL_MAX_WATCH) {
            return -LINUX_ENOSPC;
        }
        size_t idx = ep->count++;
        ep->watches[idx].fd = fd;
        ep->watches[idx].event = *event;
        ep->watches[idx].active = true;
        return 0;
    } else if (op == LINUX_EPOLL_CTL_MOD) {
        if (!event) return -LINUX_EFAULT;
        for (size_t i = 0; i < ep->count; i++) {
            if (ep->watches[i].active && ep->watches[i].fd == fd) {
                ep->watches[i].event = *event;
                return 0;
            }
        }
        return -LINUX_ENOENT;
    } else if (op == LINUX_EPOLL_CTL_DEL) {
        for (size_t i = 0; i < ep->count; i++) {
            if (ep->watches[i].active && ep->watches[i].fd == fd) {
                ep->watches[i].active = false;
                return 0;
            }
        }
        return -LINUX_ENOENT;
    }

    return -LINUX_EINVAL;
}

static uint32_t check_fd_readiness(const linux_fd_map_t *fde) {
    uint32_t revents = 0;
    if (!fde || fde->type == LINUX_FD_TYPE_NONE) {
        return LINUX_EPOLLERR;
    }

    switch (fde->type) {
        case LINUX_FD_TYPE_CONSOLE:
            revents |= LINUX_EPOLLOUT;
            break;

        case LINUX_FD_TYPE_DEV_NULL:
        case LINUX_FD_TYPE_DEV_ZERO:
        case LINUX_FD_TYPE_DEV_URANDOM:
        case LINUX_FD_TYPE_PROC:
            revents |= (LINUX_EPOLLIN | LINUX_EPOLLOUT);
            break;

        case LINUX_FD_TYPE_PIPE_READ: {
            linux_pipe_t *p = (linux_pipe_t *)fde->priv_data;
            if (p) {
                if (linux_pipe_is_readable(p)) revents |= LINUX_EPOLLIN;
                if (!p->writer_open) revents |= LINUX_EPOLLHUP;
            }
            break;
        }

        case LINUX_FD_TYPE_PIPE_WRITE: {
            linux_pipe_t *p = (linux_pipe_t *)fde->priv_data;
            if (p) {
                if (linux_pipe_is_writable(p)) revents |= LINUX_EPOLLOUT;
                if (!p->reader_open) revents |= LINUX_EPOLLERR;
            }
            break;
        }

        case LINUX_FD_TYPE_EVENTFD: {
            linux_eventfd_t *efd = (linux_eventfd_t *)fde->priv_data;
            if (efd) {
                if (linux_eventfd_is_readable(efd)) revents |= LINUX_EPOLLIN;
                if (linux_eventfd_is_writable(efd)) revents |= LINUX_EPOLLOUT;
            }
            break;
        }

        case LINUX_FD_TYPE_TIMERFD: {
            linux_timerfd_t *tfd = (linux_timerfd_t *)fde->priv_data;
            if (tfd && linux_timerfd_is_readable(tfd)) {
                revents |= LINUX_EPOLLIN;
            }
            break;
        }

        default:
            revents |= (LINUX_EPOLLIN | LINUX_EPOLLOUT);
            break;
    }
    return revents;
}

long linux_epoll_wait(linux_epoll_t *ep, linux_process_state_t *st, struct linux_epoll_event *events, int maxevents, int timeout_ms) {
    if (!ep || !st || !events || maxevents <= 0) return -LINUX_EINVAL;

    uint64_t start_ns = bh_ktime_now();
    uint64_t timeout_ns = (timeout_ms > 0) ? ((uint64_t)timeout_ms * 1000000ULL) : 0;

    while (1) {
        int ready = 0;
        for (size_t i = 0; i < ep->count; i++) {
            if (!ep->watches[i].active) continue;
            int fd = ep->watches[i].fd;
            if (fd < 0 || fd >= LINUX_MAX_FDS) continue;

            uint32_t rev = check_fd_readiness(&st->fd_table[fd]);
            uint32_t matched = rev & ep->watches[i].event.events;
            if (matched) {
                events[ready].events = matched;
                events[ready].data = ep->watches[i].event.data;
                ready++;
                if (ready >= maxevents) break;
            }
        }

        if (ready > 0 || timeout_ms == 0) {
            return ready;
        }

        if (timeout_ms > 0) {
            uint64_t elapsed = bh_ktime_now() - start_ns;
            if (elapsed >= timeout_ns) {
                return 0; // Timeout expired with 0 ready
            }
        }

        bh_thread_yield();
    }
}
