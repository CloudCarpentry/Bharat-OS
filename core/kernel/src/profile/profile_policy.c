#include "profile/profile_policy.h"
#include "bharat/personality/personality_interface.h"
#include <stddef.h>

static const bh_profile_policy_t default_policy = {
    .name = "default-native",
    .traits = BH_PROFILE_TRAIT_MMU_FULL | BH_PROFILE_TRAIT_SERVICE_RICH,
    .allowed_personalities = (1ULL << BH_PERSONALITY_NATIVE) | (1ULL << BH_PERSONALITY_LINUX),
    .syscall_policy_flags = 0,
    .max_usercopy_bytes = 4096
};

const bh_profile_policy_t *bh_profile_current_policy(void) {
    // TODO: Load from boot config/hardware profile
    return &default_policy;
}

bool bh_profile_has_trait(uint64_t trait) {
    const bh_profile_policy_t *policy = bh_profile_current_policy();
    return (policy->traits & trait) != 0;
}

bool bh_profile_allows_personality(uint32_t personality) {
    const bh_profile_policy_t *policy = bh_profile_current_policy();
    return (policy->allowed_personalities & (1ULL << personality)) != 0;
}

bool bh_profile_allows_blocking_syscall(void) {
    if (bh_profile_has_trait(BH_PROFILE_TRAIT_NO_BLOCKING_RT)) {
        // Check if current thread is RT? For now, trait-level check.
        return false;
    }
    return true;
}

bool bh_profile_requires_vma_usercopy(void) {
    return bh_profile_has_trait(BH_PROFILE_TRAIT_MMU_FULL);
}
