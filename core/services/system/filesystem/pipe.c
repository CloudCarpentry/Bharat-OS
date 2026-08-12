#include "pipe.h"
#include <stdlib.h>
#include <string.h>

#define PIPE_BUF_SIZE 4096

typedef struct {
    char buffer[PIPE_BUF_SIZE];
    size_t head;
    size_t tail;
    int read_closed;
    int write_closed;
    vfs_node_t rnode;
    vfs_node_t wnode;
} pipe_internal_t;

static vfs_operations_t pipe_read_ops;
static vfs_operations_t pipe_write_ops;

static int pipe_read(vfs_file_t* file, uint64_t offset, void* buffer, size_t size) {
    (void)offset;
    if (!file || !file->node || !buffer) return -1;
    pipe_internal_t* p = (pipe_internal_t*)file->node->fs_data;

    // Simple non-blocking read for now
    size_t available = (p->head >= p->tail) ? (p->head - p->tail) : (PIPE_BUF_SIZE - p->tail + p->head);
    if (available == 0) {
        if (p->write_closed) return 0; // EOF
        return -1; // EAGAIN
    }

    size_t read_size = (size < available) ? size : available;
    char* dst = (char*)buffer;
    for (size_t i = 0; i < read_size; i++) {
        dst[i] = p->buffer[p->tail];
        p->tail = (p->tail + 1) % PIPE_BUF_SIZE;
    }

    return read_size;
}

static int pipe_write(vfs_file_t* file, uint64_t offset, const void* buffer, size_t size) {
    (void)offset;
    if (!file || !file->node || !buffer) return -1;
    pipe_internal_t* p = (pipe_internal_t*)file->node->fs_data;

    if (p->read_closed) return -1; // EPIPE

    size_t available = (p->tail > p->head) ? (p->tail - p->head - 1) : (PIPE_BUF_SIZE - p->head + p->tail - 1);

    size_t write_size = (size < available) ? size : available;
    if (write_size == 0) return -1; // EAGAIN

    const char* src = (const char*)buffer;
    for (size_t i = 0; i < write_size; i++) {
        p->buffer[p->head] = src[i];
        p->head = (p->head + 1) % PIPE_BUF_SIZE;
    }

    return write_size;
}

static int pipe_read_close(vfs_file_t* file) {
    if (!file || !file->node) return -1;
    pipe_internal_t* p = (pipe_internal_t*)file->node->fs_data;
    p->read_closed = 1;
    if (p->write_closed) {
        free(p);
    }
    return 0;
}

static int pipe_write_close(vfs_file_t* file) {
    if (!file || !file->node) return -1;
    pipe_internal_t* p = (pipe_internal_t*)file->node->fs_data;
    p->write_closed = 1;
    if (p->read_closed) {
        free(p);
    }
    return 0;
}

static vfs_operations_t pipe_read_ops = {
    .read = pipe_read,
    .close = pipe_read_close,
};

static vfs_operations_t pipe_write_ops = {
    .write = pipe_write,
    .close = pipe_write_close,
};

int fs_pipe_create(vfs_node_t** out_read_node, vfs_node_t** out_write_node) {
    if (!out_read_node || !out_write_node) return -1;

    pipe_internal_t* p = (pipe_internal_t*)malloc(sizeof(pipe_internal_t));
    if (!p) return -1;

    memset(p, 0, sizeof(pipe_internal_t));

    p->rnode.fs_data = p;
    p->rnode.ops = &pipe_read_ops;
    p->rnode.flags = 0; // PIPEFIFO

    p->wnode.fs_data = p;
    p->wnode.ops = &pipe_write_ops;
    p->wnode.flags = 0; // PIPEFIFO

    *out_read_node = &p->rnode;
    *out_write_node = &p->wnode;

    return 0;
}
