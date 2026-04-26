#include <bharat/uapi/init/init_boot_context.h>
#include <bharat/uapi/init/init_capability.h>
#include "../../../core/services/core/init/init_profile.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

void test_all_profiles_return_policy() {
    init_profile_t profiles[] = {
        INIT_PROFILE_TINY, INIT_PROFILE_SMALL, INIT_PROFILE_EMBEDDED_RICH,
        INIT_PROFILE_RT, INIT_PROFILE_MOBILE, INIT_PROFILE_DESKTOP,
        INIT_PROFILE_DRONE, INIT_PROFILE_CLOUD, INIT_PROFILE_AUTOMOTIVE,
        INIT_PROFILE_TV, INIT_PROFILE_APPLIANCE, INIT_PROFILE_WATCH
    };
    size_t count = sizeof(profiles) / sizeof(profiles[0]);

    for (size_t i = 0; i < count; i++) {
        const init_profile_policy_t *policy = init_profile_get_policy(profiles[i]);
        assert(policy != NULL);
        assert(policy->name != NULL);
        printf("Profile %d returned policy: %s\n", profiles[i], policy->name);
    }
}

void test_unknown_profile_returns_default() {
    const init_profile_policy_t *policy = init_profile_get_policy((init_profile_t)9999);
    assert(policy != NULL);
    assert(strcmp(policy->name, "unknown") == 0);
    printf("Unknown profile returned default policy\n");
}

void test_profile_to_mask() {
    assert(init_profile_to_mask(INIT_PROFILE_TINY) == 1);
    assert(init_profile_to_mask(INIT_PROFILE_AUTOMOTIVE) == 256);
    printf("Profile to mask conversion correct\n");
}

void test_mask_allows_profile() {
    uint64_t mask = (uint64_t)INIT_PROFILE_TINY | (uint64_t)INIT_PROFILE_SMALL;
    assert(init_profile_mask_allows(mask, INIT_PROFILE_TINY) == true);
    assert(init_profile_mask_allows(mask, INIT_PROFILE_SMALL) == true);
    assert(init_profile_mask_allows(mask, INIT_PROFILE_DESKTOP) == false);
    printf("Mask allows profile helper correct\n");
}

int main() {
    test_all_profiles_return_policy();
    test_unknown_profile_returns_default();
    test_profile_to_mask();
    test_mask_allows_profile();
    printf("All profile policy tests passed!\n");
    return 0;
}
