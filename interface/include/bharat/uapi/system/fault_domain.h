/*
 * Transitional UAPI placeholder.
 * This header exists to unblock current build wiring.
 * Full contract belongs to a dedicated system-contract task.
 */
#ifndef BHARAT_UAPI_SYSTEM_FAULT_DOMAIN_H
#define BHARAT_UAPI_SYSTEM_FAULT_DOMAIN_H

#include <stdint.h>

typedef struct bharat_fault_domain_attr {
    uint32_t version;
    uint32_t flags;
} bharat_fault_domain_attr_t;

#endif // BHARAT_UAPI_SYSTEM_FAULT_DOMAIN_H
