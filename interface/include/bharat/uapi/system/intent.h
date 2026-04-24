#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BHARAT_INTENT_V1 1

typedef enum {
    BHARAT_INTENT_LATENCY_NONE = 0,
    BHARAT_INTENT_LATENCY_BEST_EFFORT = 1,
    BHARAT_INTENT_LATENCY_SENSITIVE = 2,
    BHARAT_INTENT_LATENCY_DEADLINE = 3,
} bharat_intent_latency_class_t;

typedef enum {
    BHARAT_INTENT_ENERGY_DEFAULT = 0,
    BHARAT_INTENT_ENERGY_PREFER_EFFICIENCY = 1,
    BHARAT_INTENT_ENERGY_BALANCED = 2,
    BHARAT_INTENT_ENERGY_PREFER_PERFORMANCE = 3,
} bharat_intent_energy_class_t;

typedef enum {
    BHARAT_INTENT_RELIABILITY_DEFAULT = 0,
    BHARAT_INTENT_RELIABILITY_STANDARD = 1,
    BHARAT_INTENT_RELIABILITY_HIGH = 2,
    BHARAT_INTENT_RELIABILITY_CRITICAL = 3,
} bharat_intent_reliability_class_t;

typedef enum {
    BHARAT_INTENT_ISOLATION_DEFAULT = 0,
    BHARAT_INTENT_ISOLATION_SHARED = 1,
    BHARAT_INTENT_ISOLATION_STRONG = 2,
    BHARAT_INTENT_ISOLATION_EXCLUSIVE = 3,
} bharat_intent_isolation_class_t;

typedef struct {
    uint32_t version;
    uint32_t flags;

    uint32_t latency_class;
    uint32_t energy_class;
    uint32_t reliability_class;
    uint32_t isolation_class;

    uint64_t deadline_ns;      // 0 if not specified
    uint64_t period_ns;        // 0 if not periodic
    uint64_t max_jitter_ns;    // 0 if unspecified

    uint32_t preferred_cpu_class_mask; // e.g. little/big/rt/any
    uint32_t reserved0;

    uint64_t reserved[4];
} bharat_intent_t;

#ifdef __cplusplus
}
#endif
