#ifndef BHARAT_PROFILE_POLICY_H
#define BHARAT_PROFILE_POLICY_H

#include <stdint.h>
#include <stdbool.h>

/*
 * Bharat-OS Profile Policy Traits
 * Instead of fixed enums, we use traits to describe what a profile allows.
 */

#define BH_PROFILE_TRAIT_MMU_FULL        (1ULL << 0)
#define BH_PROFILE_TRAIT_MMU_LITE        (1ULL << 1)
#define BH_PROFILE_TRAIT_MPU_ONLY        (1ULL << 2)
#define BH_PROFILE_TRAIT_HARD_RT         (1ULL << 3)
#define BH_PROFILE_TRAIT_SERVICE_RICH    (1ULL << 4)
#define BH_PROFILE_TRAIT_COMPAT_LINUX    (1ULL << 5)
#define BH_PROFILE_TRAIT_COMPAT_ANDROID  (1ULL << 6)
#define BH_PROFILE_TRAIT_AUDIT_REQUIRED  (1ULL << 7)
#define BH_PROFILE_TRAIT_LOW_MEMORY      (1ULL << 8)
#define BH_PROFILE_TRAIT_NO_BLOCKING_RT  (1ULL << 9)

typedef struct bh_profile_policy {
    const char *name;
    uint64_t traits;
    uint64_t allowed_personalities;
    uint64_t syscall_policy_flags;
    uint64_t max_usercopy_bytes;
} bh_profile_policy_t;

const bh_profile_policy_t *bh_profile_current_policy(void);

bool bh_profile_has_trait(uint64_t trait);
bool bh_profile_allows_personality(uint32_t personality);
bool bh_profile_allows_blocking_syscall(void);
bool bh_profile_requires_vma_usercopy(void);

#endif /* BHARAT_PROFILE_POLICY_H */
