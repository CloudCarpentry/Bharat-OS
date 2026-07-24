#include "win_compat.h"
#include "trap/syscall_context.h"
#include "personality_ops.h"

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

const bh_personality_syscall_table_t *personality_windows_get_table(void) {
    return NULL;
}

const personality_ops_t *personality_windows_get_ops(void) {
    return NULL;
}
