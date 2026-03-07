#include "win_compat.h"

int winnt_subsys_init(subsys_instance_t* env) {
    if (!env) {
        return -1;
    }

    if (env->type != SUBSYS_TYPE_WINDOWS) {
        return -2;
    }

    /* Stub personality: reserved for future NT syscall/PE emulation. */
    env->memory_limit_mb = (env->memory_limit_mb == 0U) ? 2048U : env->memory_limit_mb;
    return 0;
}
