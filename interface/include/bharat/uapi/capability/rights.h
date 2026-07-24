#ifndef BHARAT_UAPI_CAPABILITY_RIGHTS_H
#define BHARAT_UAPI_CAPABILITY_RIGHTS_H

#include <stdint.h>

/**
 * Bharat-OS Standard Capability Rights.
 * These are generic rights that can be applied to various kernel objects.
 * Defined as stable public bitmasks.
 */

#define BH_CAP_RIGHT_READ                 (1ULL << 31)
#define BH_CAP_RIGHT_WRITE                (1ULL << 32)
#define BH_CAP_RIGHT_EXECUTE              (1ULL << 33)
#define BH_CAP_RIGHT_CREATE               (1ULL << 3)
#define BH_CAP_RIGHT_DESTROY              (1ULL << 4)
#define BH_CAP_RIGHT_MAP                  (1ULL << 5)
#define BH_CAP_RIGHT_UNMAP                (1ULL << 6)
#define BH_CAP_RIGHT_SEND                 (1ULL << 7)
#define BH_CAP_RIGHT_RECEIVE              (1ULL << 8)
#define BH_CAP_RIGHT_DELEGATE             (1ULL << 7)
#define BH_CAP_RIGHT_CONTROL              (1ULL << 10)
#define BH_CAP_RIGHT_QUERY                (1ULL << 20)

/* Semantic rights aligned with the kernel capability rights bits */
#define BH_CAP_RIGHT_ENDPOINT_SEND        (1ULL << 0)
#define BH_CAP_RIGHT_ENDPOINT_RECEIVE     (1ULL << 1)
#define BH_CAP_RIGHT_MEMORY_MAP           (1ULL << 2)
#define BH_CAP_RIGHT_MEMORY_UNMAP         (1ULL << 3)
#define BH_CAP_RIGHT_SCHEDULE             (1ULL << 6)
#define BH_CAP_RIGHT_CRYPT_USE            (1ULL << 8)
#define BH_CAP_RIGHT_FAULT_DOMAIN_MANAGE  (1ULL << 35)
#define BH_CAP_RIGHT_PROCESS_MANAGE       (1ULL << 36)
#define BH_CAP_RIGHT_RESOURCE_ALLOC       (1ULL << 37)

#endif /* BHARAT_UAPI_CAPABILITY_RIGHTS_H */
