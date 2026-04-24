#include <assert.h>
#include <stdio.h>

#include "secure_boot.h"

int hal_secure_boot_arch_check(const bharat_boot_policy_t* policy) {
    return policy ? 0 : -1;
}

int main(void) {
    const bharat_boot_policy_t* policy = bharat_boot_active_policy();
    assert(policy != NULL);
    assert(policy->timer_tick_hz > 0U);
    assert(policy->smp_target_cores > 0U);

    if (policy->security_level != BHARAT_BOOT_SECURITY_DISABLED) {
        assert(bharat_secure_boot_verify_early() == 0);
    }

    printf("secure_boot_policy: pass\n");
    return 0;
}
