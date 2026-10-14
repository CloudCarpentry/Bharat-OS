/* SPDX-License-Identifier: MIT */
#include "linux_pipe.h"
#include "linux_errno.h"
#include "syscall/usercopy.h"
#include "sched/sched.h"
#include "lib/base/string.h"

#define MAX_LINUX_PIPES 64

static linux_pipe_t g_linux_pipe_pool[MAX_LINUX_PIPES];

linux_pipe_t *linux_pipe_alloc(void) {
    for (int i = 0; i < MAX_LINUX_PIPES; i++) {
        if (!g_linux_pipe_pool[i].in_use) {
            memset(&g_linux_pipe_pool[i], 0, sizeof(linux_pipe_t));
            g_linux_pipe_pool[i].in_use = 1;
            g_linux_pipe_pool[i].ref_count = 2; // 1 for read end, 1 for write end
            g_linux_pipe_pool[i].reader_open = true;
            g_linux_pipe_pool[i].writer_open = true;
            return &g_linux_pipe_pool[i];
        }
    }
    return NULL;
}

void linux_pipe_ref(linux_pipe_t *pipe) {
    if (pipe) {
        pipe->ref_count++;
    }
}

void linux_pipe_unref(linux_pipe_t *pipe) {
    if (!pipe) return;
    pipe->ref_count--;
    if (pipe->ref_count <= 0) {
        pipe->in_use = 0;
        pipe->reader_open = false;
        pipe->writer_open = false;
        pipe->count = 0;
    }
}

void linux_pipe_close_read(linux_pipe_t *pipe) {
    if (pipe) {
        pipe->reader_open = false;
        linux_pipe_unref(pipe);
    }
}

void linux_pipe_close_write(linux_pipe_t *pipe) {
    if (pipe) {
        pipe->writer_open = false;
        linux_pipe_unref(pipe);
    }
}

bool linux_pipe_is_readable(linux_pipe_t *pipe) {
    if (!pipe) return false;
    // Readable if data available or writer has closed (EOF)
    return (pipe->count > 0) || (!pipe->writer_open);
}

bool linux_pipe_is_writable(linux_pipe_t *pipe) {
    if (!pipe) return false;
    // Writable if buffer space available and reader still open
    return (pipe->count < LINUX_PIPE_BUF_SIZE) || (!pipe->reader_open);
}

long linux_pipe_read(linux_pipe_t *pipe, void *buf, size_t count, bool nonblock) {
    if (!pipe || !buf) return -LINUX_EFAULT;
    if (count == 0) return 0;

    while (pipe->count == 0) {
        if (!pipe->writer_open) {
            return 0; // EOF
        }
        if (nonblock) {
            return -LINUX_EAGAIN;
        }
        bh_thread_yield();
    }

    size_t to_read = count;
    if (to_read > pipe->count) {
        to_read = pipe->count;
    }

    size_t copied = 0;
    while (copied < to_read) {
        size_t chunk = to_read - copied;
        size_t avail_contiguous = LINUX_PIPE_BUF_SIZE - pipe->head;
        if (chunk > avail_contiguous) {
            chunk = avail_contiguous;
        }

        bh_status_t s = bh_copy_to_user((uint8_t *)buf + copied, pipe->buffer + pipe->head, chunk);
        if (s != BH_OK) {
            return copied ? (long)copied : -LINUX_EFAULT;
        }

        pipe->head = (pipe->head + chunk) % LINUX_PIPE_BUF_SIZE;
        pipe->count -= chunk;
        copied += chunk;
    }

    return (long)copied;
}

long linux_pipe_write(linux_pipe_t *pipe, const void *buf, size_t count, bool nonblock) {
    if (!pipe || !buf) return -LINUX_EFAULT;
    if (count == 0) return 0;

    if (!pipe->reader_open) {
        return -LINUX_EPIPE;
    }

    size_t written = 0;
    while (written < count) {
        while (pipe->count == LINUX_PIPE_BUF_SIZE) {
            if (!pipe->reader_open) {
                return written ? (long)written : -LINUX_EPIPE;
            }
            if (nonblock) {
                return written ? (long)written : -LINUX_EAGAIN;
            }
            bh_thread_yield();
        }

        size_t avail = LINUX_PIPE_BUF_SIZE - pipe->count;
        size_t chunk = count - written;
        if (chunk > avail) {
            chunk = avail;
        }

        size_t avail_contiguous = LINUX_PIPE_BUF_SIZE - pipe->tail;
        if (chunk > avail_contiguous) {
            chunk = avail_contiguous;
        }

        bh_status_t s = bh_copy_from_user(pipe->buffer + pipe->tail, (const uint8_t *)buf + written, chunk);
        if (s != BH_OK) {
            return written ? (long)written : -LINUX_EFAULT;
        }

        pipe->tail = (pipe->tail + chunk) % LINUX_PIPE_BUF_SIZE;
        pipe->count += chunk;
        written += chunk;
    }

    return (long)written;
}
