#ifndef BHARAT_UAPI_CAPABILITY_RIGHTS_H
#define BHARAT_UAPI_CAPABILITY_RIGHTS_H

#include <stdint.h>

/**
 * Bharat-OS Standard Capability Rights.
 * These are generic rights that can be applied to various kernel objects.
 */

#define BH_CAP_RIGHT_READ        (1ULL << 0)
#define BH_CAP_RIGHT_WRITE       (1ULL << 1)
#define BH_CAP_RIGHT_EXECUTE     (1ULL << 2)
#define BH_CAP_RIGHT_CREATE      (1ULL << 3)
#define BH_CAP_RIGHT_DESTROY     (1ULL << 4)
#define BH_CAP_RIGHT_MAP         (1ULL << 5)
#define BH_CAP_RIGHT_UNMAP       (1ULL << 6)
#define BH_CAP_RIGHT_SEND        (1ULL << 7)
#define BH_CAP_RIGHT_RECEIVE     (1ULL << 8)
#define BH_CAP_RIGHT_DELEGATE    (1ULL << 9)
#define BH_CAP_RIGHT_CONTROL     (1ULL << 10)
#define BH_CAP_RIGHT_QUERY       (1ULL << 11)

#endif /* BHARAT_UAPI_CAPABILITY_RIGHTS_H */
