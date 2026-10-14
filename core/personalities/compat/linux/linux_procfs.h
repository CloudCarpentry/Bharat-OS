/* SPDX-License-Identifier: MIT */
#ifndef BHARAT_LINUX_PROCFS_H
#define BHARAT_LINUX_PROCFS_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define LINUX_PROCFS_MAX_CONTENT 2048

typedef struct linux_procfs_file {
    char data[LINUX_PROCFS_MAX_CONTENT];
    size_t size;
    int ref_count;
    int in_use;
} linux_procfs_file_t;

struct bh_process;

linux_procfs_file_t *linux_procfs_generate(const char *path, struct bh_process *proc);
void linux_procfs_file_ref(linux_procfs_file_t *pf);
void linux_procfs_file_unref(linux_procfs_file_t *pf);
long linux_procfs_read(linux_procfs_file_t *pf, uint64_t *offset, void *buf, size_t count);

#endif /* BHARAT_LINUX_PROCFS_H */
