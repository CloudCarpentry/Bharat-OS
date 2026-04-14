#ifndef BHARAT_INIT_PROFILE_H
#define BHARAT_INIT_PROFILE_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    BHARAT_INIT_PROFILE_TINY = 0,
    BHARAT_INIT_PROFILE_SMALL,
    BHARAT_INIT_PROFILE_EMBEDDED_RICH,
    BHARAT_INIT_PROFILE_MOBILE,
    BHARAT_INIT_PROFILE_DESKTOP,
    BHARAT_INIT_PROFILE_DRONE,
} bharat_init_profile_t;

typedef uint64_t bharat_init_profile_mask_t;
typedef uint64_t bharat_init_cap_mask_t;
typedef uint64_t bharat_init_board_mask_t;
typedef uint64_t bharat_init_personality_mask_t;

/* Bitmask generation helpers */
#define INIT_PROFILE_MASK(profile) (1ULL << (profile))
#define INIT_PROFILE_MASK_ALL (~0ULL)

/* Helper to determine the active bootstrap profile */
bharat_init_profile_t init_profile_get_active(void);

#endif /* BHARAT_INIT_PROFILE_H */
