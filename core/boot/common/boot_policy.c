#include "hal/hal_boot.h"

// Parse and set up profile toggles

int boot_policy_apply(void) {
    bharat_boot_info_t* boot_info = hal_boot_get_info();
    if (!boot_info) return -1;

    // Based on parsed command line or predefined profile enum,
    // we set the struct toggles.
    // For now, we set some safe defaults if not initialized.

    if (boot_info->profile_toggles.strict_secure_boot_required) {
        boot_info->profile_toggles.unsigned_module_loading_disabled = true;
    }

    // Set preference for one-shot timers vs periodic
    // (A multikernel design defaults to one-shot per core)
    boot_info->profile_toggles.timer_preference_oneshot = true;

    // If SMP is not allowed, restrict to 1 CPU logically
    if (!boot_info->profile_toggles.smp_allowed && boot_info->cpu_count > 1) {
        boot_info->cpu_count = 1;
    }

    return 0;
}
