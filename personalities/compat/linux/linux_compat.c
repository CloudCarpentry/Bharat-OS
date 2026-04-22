void linux_vfs_init(void);
#include "linux_compat.h"

#include <stddef.h>

#include "personality/translation_events.h"

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

long linux_syscall_handler(long sysno, long arg1, long arg2, long arg3, long arg4, long arg5, long arg6) {
    (void)arg1;
    (void)arg2;
    (void)arg3;
    (void)arg4;
    (void)arg5;
    (void)arg6;

    bh_translation_event_record(BH_TRANSLATION_EVENT_BOUNDARY_ENTER);

    if (!g_linux_instance || !g_linux_instance->is_running) {
        bh_translation_event_record(BH_TRANSLATION_EVENT_FALLBACK);
        LINUX_RETURN(-38);
    }

    switch (sysno) {
        case 0:
            if (arg1 >= 0 && arg1 < LINUX_MAX_FDS && fd_table[arg1].type != LINUX_FD_TYPE_NONE) {
                if (fd_table[arg1].type == LINUX_FD_TYPE_CONSOLE) {
                    bh_translation_event_record(BH_TRANSLATION_EVENT_FALLBACK);
                    LINUX_RETURN(-38);
                }
                bh_translation_event_record(BH_TRANSLATION_EVENT_FALLBACK);
                LINUX_RETURN(-38);
            }
            bh_translation_event_record(BH_TRANSLATION_EVENT_CACHE_MISS);
            LINUX_RETURN(-9);
        case 1:
            if (arg1 >= 0 && arg1 < LINUX_MAX_FDS && fd_table[arg1].type != LINUX_FD_TYPE_NONE) {
                if (fd_table[arg1].type == LINUX_FD_TYPE_CONSOLE) {
                    LINUX_RETURN(arg3);
                }
                bh_translation_event_record(BH_TRANSLATION_EVENT_FALLBACK);
                LINUX_RETURN(-38);
            }
            bh_translation_event_record(BH_TRANSLATION_EVENT_CACHE_MISS);
            LINUX_RETURN(-9);
        case 2:
        case 257: {
            int fd = next_fd++;
            if (fd >= LINUX_MAX_FDS) {
                bh_translation_event_record(BH_TRANSLATION_EVENT_FALLBACK);
                LINUX_RETURN(-24);
            }
            linux_map_fd_to_capability(g_linux_instance, fd, 0, LINUX_FD_TYPE_FILE);
            LINUX_RETURN(fd);
        }
        case 3:
            if (arg1 >= 0 && arg1 < LINUX_MAX_FDS && fd_table[arg1].type != LINUX_FD_TYPE_NONE) {
                fd_table[arg1].ref_count--;
                if (fd_table[arg1].ref_count <= 0) {
                    fd_table[arg1].type = LINUX_FD_TYPE_NONE;
                    fd_table[arg1].linux_fd = -1;
                    fd_table[arg1].backing_capability = 0;
                }
                LINUX_RETURN(0);
            }
            bh_translation_event_record(BH_TRANSLATION_EVENT_CACHE_MISS);
            LINUX_RETURN(-9);
        case 9:
        case 10:
        case 11:
        case 12:
        case 13:
        case 14:
        case 57:
        case 59:
        case 63:
        case 202:
        case 228:
        case 230:
            bh_translation_event_record(BH_TRANSLATION_EVENT_FALLBACK);
            LINUX_RETURN(-38);
        case 16:
            if (arg1 >= 0 && arg1 < LINUX_MAX_FDS && fd_table[arg1].type == LINUX_FD_TYPE_CONSOLE) {
                bh_translation_event_record(BH_TRANSLATION_EVENT_FALLBACK);
                LINUX_RETURN(-38);
            }
            LINUX_RETURN(-25);
        case 39:
        case 186:
            LINUX_RETURN(1);
        case 60:
        case 231:
            g_linux_instance->is_running = 0;
            LINUX_RETURN(0);
        default:
            bh_translation_event_record(BH_TRANSLATION_EVENT_FALLBACK);
            LINUX_RETURN(-38);
    }
}
