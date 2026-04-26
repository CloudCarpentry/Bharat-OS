#ifndef BHARAT_UAPI_INIT_CAPABILITY_H
#define BHARAT_UAPI_INIT_CAPABILITY_H

#include <stdint.h>

typedef uint64_t bharat_init_cap_mask_t;

#define BHARAT_INIT_CAP_NONE      ((bharat_init_cap_mask_t)0)
#define BHARAT_INIT_CAP_NETWORK   ((bharat_init_cap_mask_t)(1ULL << 0))
#define BHARAT_INIT_CAP_STORAGE   ((bharat_init_cap_mask_t)(1ULL << 1))
#define BHARAT_INIT_CAP_DISPLAY   ((bharat_init_cap_mask_t)(1ULL << 2))
#define BHARAT_INIT_CAP_SENSORS   ((bharat_init_cap_mask_t)(1ULL << 3))
#define BHARAT_INIT_CAP_MMU       ((bharat_init_cap_mask_t)(1ULL << 4))
#define BHARAT_INIT_CAP_CAN       ((bharat_init_cap_mask_t)(1ULL << 5))
#define BHARAT_INIT_CAP_ACTUATOR  ((bharat_init_cap_mask_t)(1ULL << 6))
#define BHARAT_INIT_CAP_POWER     ((bharat_init_cap_mask_t)(1ULL << 7))
#define BHARAT_INIT_CAP_ACCEL     ((bharat_init_cap_mask_t)(1ULL << 8))

#endif // BHARAT_UAPI_INIT_CAPABILITY_H
