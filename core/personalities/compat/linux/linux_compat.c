void linux_vfs_init(void);
#include "linux_compat.h"

#include <stddef.h>

#include "personality/translation_events.h"
#include "trap/syscall_regs.h"
#include "trap/syscall_context.h"

extern const bh_personality_syscall_table_t *linux_personality_get_table(void);

static subsys_instance_t* g_linux_instance;

#define LINUX_RETURN(value)                           \
    do {                                              \
        bh_translation_event_record(BH_TRANSLATION_EVENT_BOUNDARY_EXIT); \
        return (value);                               \
    } while (0)

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
    linux_vfs_init();
    return 0;
}

#include "bharat/subsys_test.h"

long linux_syscall_handler(long sysno, long arg1, long arg2, long arg3, long arg4, long arg5, long arg6);

static int test_linux_syscall_sanity(void) {
    if (linux_syscall_handler(39, 0, 0, 0, 0, 0, 0) == -38) {
        subsys_instance_t dummy_env = { .type = SUBSYS_TYPE_LINUX, .is_running = 1 };
        g_linux_instance = &dummy_env;
        if (linux_syscall_handler(39, 0, 0, 0, 0, 0, 0) == 1) {
            g_linux_instance = NULL;
            return 0;
        }
        g_linux_instance = NULL;
        return -1;
    }
    return -1;
}

REGISTER_SUBSYS_TEST("linux", "syscall_translation_sanity", test_linux_syscall_sanity, 1, 1)

// Transitional helper to route legacy calls through the new descriptor-based gate logic
long linux_syscall_handler(long sysno, long arg1, long arg2, long arg3, long arg4, long arg5, long arg6) {
    bh_syscall_ctx_t ctx = {0};
    ctx.personality = BH_PERSONALITY_LINUX;
    ctx.regs.nr = (uintptr_t)sysno;
    ctx.regs.arg[0] = (uintptr_t)arg1;
    ctx.regs.arg[1] = (uintptr_t)arg2;
    ctx.regs.arg[2] = (uintptr_t)arg3;
    ctx.regs.arg[3] = (uintptr_t)arg4;
    ctx.regs.arg[4] = (uintptr_t)arg5;
    ctx.regs.arg[5] = (uintptr_t)arg6;

    if (!g_linux_instance || !g_linux_instance->is_running) {
        return -38; // ENOSYS equivalent
    }

    const bh_personality_syscall_table_t *table = linux_personality_get_table();
    if (!table || ctx.regs.nr > table->max_syscall_nr) {
        return -38;
    }

    const bh_syscall_desc_t *desc = &table->table[ctx.regs.nr];
    if (!desc || !desc->handler) {
        return -38;
    }

    return desc->handler(&ctx);
}

// Rest of VFS and FD mapping...
static linux_fd_map_t fd_table[LINUX_MAX_FDS];
static int next_fd = 3;

int linux_map_fd_to_capability(subsys_instance_t* env, int linux_fd, uint32_t cap, linux_fd_type_t type) {
    if (!env || linux_fd < 0 || linux_fd >= LINUX_MAX_FDS) return -1;
    fd_table[linux_fd].type = type;
    fd_table[linux_fd].linux_fd = linux_fd;
    fd_table[linux_fd].backing_capability = cap;
    fd_table[linux_fd].open_flags = 0;
    fd_table[linux_fd].file_offset = 0;
    fd_table[linux_fd].ref_count = 1;
    return 0;
}

void linux_vfs_init(void) {
    for (int i = 0; i < LINUX_MAX_FDS; i++) {
        fd_table[i].type = LINUX_FD_TYPE_NONE;
        fd_table[i].ref_count = 0;
    }
    linux_map_fd_to_capability(g_linux_instance, 0, 0, LINUX_FD_TYPE_CONSOLE);
    linux_map_fd_to_capability(g_linux_instance, 1, 0, LINUX_FD_TYPE_CONSOLE);
    linux_map_fd_to_capability(g_linux_instance, 2, 0, LINUX_FD_TYPE_CONSOLE);
}
