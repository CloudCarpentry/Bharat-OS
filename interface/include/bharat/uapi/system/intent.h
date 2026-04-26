/*
 * Transitional UAPI placeholder.
 * This header exists to unblock current build wiring.
 * Full contract belongs to a dedicated system-contract task.
 */
#ifndef BHARAT_UAPI_SYSTEM_INTENT_H
#define BHARAT_UAPI_SYSTEM_INTENT_H

#include <stdint.h>

#define BHARAT_INTENT_V1 1

typedef struct bharat_intent {
    uint32_t version;
    uint32_t flags;
    uint64_t data[4];
} bharat_intent_t;

#endif // BHARAT_UAPI_SYSTEM_INTENT_H
