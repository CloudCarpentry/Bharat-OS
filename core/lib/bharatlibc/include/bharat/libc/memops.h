#ifndef BHARAT_LIBC_MEMOPS_H
#define BHARAT_LIBC_MEMOPS_H

#include <standard/stddef.h>
#include <standard/stdint.h>
#include <stdbool.h>

#ifndef BHARAT_CPU_FEATURES_V1_H
#define BHARAT_CPU_FEATURES_V1_H
#define BHARAT_CPU_FEATURES_ABI_V1 1u
#define BHARAT_CPU_FEATURE_WORDS 4u
#define BHARAT_CPU_FEATURE_FAST_STRING 0u

/* Safe-on-all-schedulable-CPUs descriptor supplied by the Bharat CRT. */
typedef struct {
    uint16_t abi_version;
    uint16_t size;
    uint32_t reserved;
    uint64_t feature_words[BHARAT_CPU_FEATURE_WORDS];
    uint32_t cache_line_size;
    uint32_t preferred_copy_alignment;
} bharat_cpu_features_v1_t;
#endif

/* One-way initialization; invalid or repeated publication fails closed. */
bool bh_libc_memops_init(const bharat_cpu_features_v1_t *features);

#endif
