#ifndef BHARAT_UAPI_SYSTEM_INTENT_H
#define BHARAT_UAPI_SYSTEM_INTENT_H
#include <stdint.h>
#define BHARAT_INTENT_V1 1
typedef struct {
    uint32_t version;
    uint32_t priority;
    uint64_t deadline_ns;
    uint32_t flags;
} bharat_intent_t;
#endif
