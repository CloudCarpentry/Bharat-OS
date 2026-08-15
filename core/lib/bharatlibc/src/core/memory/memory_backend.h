#pragma once

#include <bharat/libc/memops.h>

typedef struct {
    void *(*copy)(void *, const void *, size_t);
    void *(*move)(void *, const void *, size_t);
    void *(*set)(void *, int, size_t);
    int (*compare)(const void *, const void *, size_t);
} bh_libc_memops_backend_t;

const bh_libc_memops_backend_t *
bh_libc_arch_memops_select(const bharat_cpu_features_v1_t *features);
