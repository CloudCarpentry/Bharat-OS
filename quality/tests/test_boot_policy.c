#include <stdio.h>
#include <assert.h>
#include "hal/hal_boot.h"

static bharat_boot_info_t mock_boot_info = {
    .cpu_count = 4,
    .cpus = { { .cpu_id = 0 }, { .cpu_id = 1 }, { .cpu_id = 2 }, { .cpu_id = 3 } },
    .profile_toggles = {
        .smp_allowed = false,
        .strict_secure_boot_required = true,
        .unsigned_module_loading_disabled = false // should be set by policy
    },
    .trust_evidence = {
        .trust_state = BHARAT_TRUST_MEASURED,
        .verified_stages_bitmap = 0
    }
};

bharat_boot_info_t* hal_boot_get_info(void) {
    return &mock_boot_info;
}

extern int boot_trust_verify_evidence(void);
extern int boot_policy_apply(void);

int main(void) {
    printf("[TEST] Running Boot Policy & Trust Tests...\n");

    // Since trust is measured but profile requires verified, it should fail
    int res = boot_trust_verify_evidence();
    assert(res == -1);

    // Apply policy toggles
    res = boot_policy_apply();
    assert(res == 0);

    // SMP is disabled in toggles, cpu_count should become 1
    assert(mock_boot_info.cpu_count == 1);
    assert(mock_boot_info.profile_toggles.unsigned_module_loading_disabled == true);
    assert(mock_boot_info.profile_toggles.timer_preference_oneshot == true);

    printf("[TEST] Boot Policy & Trust Passed.\n");
    return 0;
}
