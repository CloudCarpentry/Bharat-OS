#ifndef BHARAT_CPU_FEATURES_V1_H
#define BHARAT_CPU_FEATURES_V1_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define BHARAT_CPU_FEATURES_ABI_V1 1u
#define BHARAT_CPU_FEATURE_WORDS 4u
#define BHARAT_CPU_FEATURE_FAST_STRING     0u
#define BHARAT_CPU_FEATURE_AES             1u
#define BHARAT_CPU_FEATURE_SHA             2u
#define BHARAT_CPU_FEATURE_PMULL           3u
#define BHARAT_CPU_FEATURE_VECTOR          4u
#define BHARAT_CPU_FEATURE_SCALABLE_VECTOR 5u
#define BHARAT_CPU_FEATURE_CRC32           6u
#define BHARAT_CPU_FEATURE_CRC32C          7u
#define BHARAT_CPU_FEATURE_BITMANIP        8u
#define BHARAT_CPU_FEATURE_STRONG_ATOMICS  9u

#define BHARAT_CPU_FEAT_BIT_FAST_STRING     (1ULL << BHARAT_CPU_FEATURE_FAST_STRING)
#define BHARAT_CPU_FEAT_BIT_AES             (1ULL << BHARAT_CPU_FEATURE_AES)
#define BHARAT_CPU_FEAT_BIT_SHA             (1ULL << BHARAT_CPU_FEATURE_SHA)
#define BHARAT_CPU_FEAT_BIT_PMULL           (1ULL << BHARAT_CPU_FEATURE_PMULL)
#define BHARAT_CPU_FEAT_BIT_VECTOR          (1ULL << BHARAT_CPU_FEATURE_VECTOR)
#define BHARAT_CPU_FEAT_BIT_SCALABLE_VECTOR (1ULL << BHARAT_CPU_FEATURE_SCALABLE_VECTOR)
#define BHARAT_CPU_FEAT_BIT_CRC32           (1ULL << BHARAT_CPU_FEATURE_CRC32)
#define BHARAT_CPU_FEAT_BIT_CRC32C          (1ULL << BHARAT_CPU_FEATURE_CRC32C)
#define BHARAT_CPU_FEAT_BIT_BITMANIP        (1ULL << BHARAT_CPU_FEATURE_BITMANIP)
#define BHARAT_CPU_FEAT_BIT_STRONG_ATOMICS  (1ULL << BHARAT_CPU_FEATURE_STRONG_ATOMICS)

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
