#include "profile.h"
#include <stddef.h>

static const char *k_profile_names[] = {
    [PROFILE_KERNEL_RT]  = "RT",
    [PROFILE_KERNEL_GP]  = "GP",
    [PROFILE_KERNEL_MIX] = "MIX",
};

bool kernel_execution_profile_is_valid(KernelExecutionProfile profile) {
    return profile == PROFILE_KERNEL_RT ||
           profile == PROFILE_KERNEL_GP ||
           profile == PROFILE_KERNEL_MIX;
}

const char *kernel_execution_profile_name(KernelExecutionProfile profile) {
    if (!kernel_execution_profile_is_valid(profile)) {
        return "UNKNOWN";
    }
    return k_profile_names[profile];
}
