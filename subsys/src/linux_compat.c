#include "linux_compat.h"

#include <stddef.h>

static subsys_instance_t* g_linux_instance;

int linux_subsys_init(subsys_instance_t* env) {
    if (!env) {
        return -1;
    }

    if (env->type != SUBSYS_TYPE_LINUX) {
        return -2;
    }

    env->memory_limit_mb = (env->memory_limit_mb == 0U) ? 1024U : env->memory_limit_mb;
    if (env->cpu_core_allocation_mask == 0U) {
        env->cpu_core_allocation_mask = 0x1U;
    }

    g_linux_instance = env;
    return 0;
}

int linux_syscall_handler(long sysno, long arg1, long arg2, long arg3, long arg4, long arg5, long arg6) {
    (void)arg1;
    (void)arg2;
    (void)arg3;
    (void)arg4;
    (void)arg5;
    (void)arg6;

    if (!g_linux_instance || !g_linux_instance->is_running) {
        return -38; /* ENOSYS-style while subsystem inactive. */
    }

    switch (sysno) {
        case 39: /* getpid */
            return 1;
        case 60: /* exit */
            g_linux_instance->is_running = 0;
            return 0;
        default:
            return -38;
    }
}
