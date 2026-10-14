/* SPDX-License-Identifier: MIT */
#ifndef BHARAT_LINUX_PIPE_H
#define BHARAT_LINUX_PIPE_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define LINUX_PIPE_BUF_SIZE 4096

typedef struct linux_pipe {
    char buffer[LINUX_PIPE_BUF_SIZE];
    size_t head;
    size_t tail;
    size_t count;
    bool reader_open;
    bool writer_open;
    int ref_count;
    int in_use;
} linux_pipe_t;

linux_pipe_t *linux_pipe_alloc(void);
void linux_pipe_ref(linux_pipe_t *pipe);
void linux_pipe_unref(linux_pipe_t *pipe);

long linux_pipe_read(linux_pipe_t *pipe, void *buf, size_t count, bool nonblock);
long linux_pipe_write(linux_pipe_t *pipe, const void *buf, size_t count, bool nonblock);
void linux_pipe_close_read(linux_pipe_t *pipe);
void linux_pipe_close_write(linux_pipe_t *pipe);
bool linux_pipe_is_readable(linux_pipe_t *pipe);
bool linux_pipe_is_writable(linux_pipe_t *pipe);

#endif /* BHARAT_LINUX_PIPE_H */
