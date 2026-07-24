#ifndef BHARAT_ARCH_CAP_PROFILE_H
#define BHARAT_ARCH_CAP_PROFILE_H

#include <arch/arch_caps.h>

typedef enum arch_runtime_tier {
    ARCH_RUNTIME_TIER1_FULL = 1,
    ARCH_RUNTIME_TIER2_EDGE32 = 2
} arch_runtime_tier_t;

typedef struct arch_cap_profile {
    arch_runtime_tier_t tier;
    arch_caps_t required;
    arch_caps_t optional;
} arch_cap_profile_t;

const arch_cap_profile_t *arch_get_cap_profile(void);

#endif /* BHARAT_ARCH_CAP_PROFILE_H */
