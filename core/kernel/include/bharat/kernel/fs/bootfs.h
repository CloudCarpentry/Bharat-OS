#ifndef BHARAT_KERNEL_FS_BOOTFS_H
#define BHARAT_KERNEL_FS_BOOTFS_H

#include <stdint.h>
#include <stddef.h>
#include "kernel/status.h"

typedef struct bh_boot_resource {
    void *data;
    size_t size;
} bh_boot_resource_t;

kstatus_t bh_boot_resource_lookup(const char *name, bh_boot_resource_t *res);

#endif
