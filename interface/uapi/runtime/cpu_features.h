#ifndef BHARAT_CPU_FEATURES_V1_H
#define BHARAT_CPU_FEATURES_V1_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define BHARAT_CPU_FEATURES_ABI_V1 1u
#define BHARAT_CPU_FEATURE_WORDS 4u
#define BHARAT_CPU_FEATURE_FAST_STRING 0u

typedef struct {
    uint16_t abi_version;
    uint16_t size;
    uint32_t reserved;
    uint64_t feature_words[BHARAT_CPU_FEATURE_WORDS];
    uint32_t cache_line_size;
    uint32_t preferred_copy_alignment;
} bharat_cpu_features_v1_t;

_Static_assert(sizeof(bharat_cpu_features_v1_t) == 48u,
               "CPU feature UAPI layout changed");

#endif
