#ifndef BHARAT_UAPI_CAPABILITY_H
#define BHARAT_UAPI_CAPABILITY_H

#include <stdint.h>

typedef uint64_t bh_cap_t;

#define BH_CAP_INVALID 0

/* Capability types */
#define BH_CAP_TYPE_NONE        0
#define BH_CAP_TYPE_THREAD      1
#define BH_CAP_TYPE_ASpace      2
#define BH_CAP_TYPE_VNODE       3
#define BH_CAP_TYPE_ENDPOINT    4
#define BH_CAP_TYPE_REPLY       5
#define BH_CAP_TYPE_DMA_GRANT   6
#define BH_CAP_TYPE_DMA_DOMAIN  7

/* Capability rights */
#define BH_CAP_RIGHT_READ       (1 << 0)
#define BH_CAP_RIGHT_WRITE      (1 << 1)
#define BH_CAP_RIGHT_EXECUTE    (1 << 2)
#define BH_CAP_RIGHT_GRANT      (1 << 3)
#define BH_CAP_RIGHT_TRANSFER   (1 << 4)

#endif /* BHARAT_UAPI_CAPABILITY_H */

#define BH_CAP_TYPE_HMEM 8

#define BH_CAP_RIGHT_MAP_CPU    (1 << 5)
#define BH_CAP_RIGHT_MAP_DEVICE (1 << 6)
#define BH_CAP_RIGHT_SHARE      (1 << 7)
#define BH_CAP_RIGHT_QUERY      (1 << 8)
#define BH_CAP_RIGHT_DESTROY    (1 << 9)
#define BH_CAP_RIGHT_RESOURCE_ALLOC (1 << 10)
