void linux_vfs_init(void);
#include "linux_compat.h"
#include "linux_errno.h"
#include "linux_pipe.h"
#include "linux_eventfd.h"
#include "linux_timerfd.h"
#include "linux_epoll.h"
#include "linux_procfs.h"

#include <stddef.h>

#include "personality/translation_events.h"
#include "trap/syscall_regs.h"
#include "trap/syscall_context.h"

extern const bh_personality_syscall_table_t bh_linux_syscall_table;
extern long bh_syscall_gate(trap_frame_t *frame, const trap_info_t *info);
struct bh_process *sched_current_process(void);
struct bh_thread *sched_current_thread(void);

const bh_personality_syscall_table_t *personality_linux_get_table(void) {
    return &bh_linux_syscall_table;
}

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

/* Transitional legacy switch. New Linux personality syscalls must use
 * linux_syscall.c descriptor table through bh_syscall_gate().
 */
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
    ctx.thread = sched_current_thread();
    ctx.process = sched_current_process();

    if (!g_linux_instance || !g_linux_instance->is_running) {
        return -38; // ENOSYS equivalent
    }

    const bh_personality_syscall_table_t *table = personality_linux_get_table();
    if (!table || ctx.regs.nr >= table->entry_count) {
        return -38;
    }

    const bh_syscall_meta_t *desc = &table->table[ctx.regs.nr];
    if (!desc || !desc->handler) {
        return -38;
    }
    ctx.desc = desc;

    bh_operation_result_t res = desc->handler(&ctx);
    if (res.domain == BH_STATUS_DOMAIN_KSTATUS) {
        return -linux_errno_from_bh_status((kstatus_t)res.value);
    }
    return (long)res.value;
}

static long linux_handle_syscall(bh_thread_t *thread, trap_frame_t *frame, const trap_info_t *info) {
    (void)thread;
    return bh_syscall_gate(frame, info);
}

static int linux_handle_user_fault(bh_thread_t *thread, trap_frame_t *frame, const trap_info_t *info) {
    (void)thread; (void)frame; (void)info;
    return -1;
}

static int linux_map_fault_to_signal(const trap_info_t *info) {
    (void)info;
    return 11; // SIGSEGV
}

static long linux_normalize_syscall_return(bh_operation_result_t result) {
    if (result.domain == BH_STATUS_DOMAIN_KSTATUS) {
        return -linux_errno_from_bh_status((kstatus_t)result.value);
    }
    return (long)result.value;
}

static const personality_ops_t linux_personality_ops = {
    .handle_syscall = linux_handle_syscall,
    .handle_user_fault = linux_handle_user_fault,
    .map_fault_to_signal = linux_map_fault_to_signal,
    .normalize_syscall_return = linux_normalize_syscall_return,
};

const personality_ops_t *personality_linux_get_ops(void) {
    return &linux_personality_ops;
}

#include "lib/base/string.h"
#include "sched/sched.h"

#define MAX_LINUX_PROCS 64
static linux_process_state_t g_linux_proc_states[MAX_LINUX_PROCS];
static uint64_t g_linux_proc_pids[MAX_LINUX_PROCS];
static int g_linux_proc_in_use[MAX_LINUX_PROCS];

linux_process_state_t *linux_get_or_create_process_state(struct bh_process *proc) {
    if (!proc) return NULL;
    if (proc->personality_data != NULL) {
        return (linux_process_state_t *)proc->personality_data;
    }

    /* Find existing entry by process_id or empty slot */
    int empty_idx = -1;
    for (int i = 0; i < MAX_LINUX_PROCS; i++) {
        if (g_linux_proc_in_use[i] && g_linux_proc_pids[i] == proc->process_id) {
            proc->personality_data = &g_linux_proc_states[i];
            return &g_linux_proc_states[i];
        }
        if (!g_linux_proc_in_use[i] && empty_idx == -1) {
            empty_idx = i;
        }
    }

    if (empty_idx == -1) {
        return NULL;
    }

    g_linux_proc_in_use[empty_idx] = 1;
    g_linux_proc_pids[empty_idx] = proc->process_id;
    linux_process_state_t *st = &g_linux_proc_states[empty_idx];
    memset(st, 0, sizeof(*st));

    for (int i = 0; i < LINUX_MAX_FDS; i++) {
        st->fd_table[i].type = LINUX_FD_TYPE_NONE;
        st->fd_table[i].ref_count = 0;
    }

    /* Standard stdio fds */
    st->fd_table[0].type = LINUX_FD_TYPE_CONSOLE;
    st->fd_table[0].linux_fd = 0;
    st->fd_table[0].ref_count = 1;

    st->fd_table[1].type = LINUX_FD_TYPE_CONSOLE;
    st->fd_table[1].linux_fd = 1;
    st->fd_table[1].ref_count = 1;

    st->fd_table[2].type = LINUX_FD_TYPE_CONSOLE;
    st->fd_table[2].linux_fd = 2;
    st->fd_table[2].ref_count = 1;

    st->next_fd = 3;
    st->brk_start = proc->brk_start;
    st->brk_current = proc->brk_current;
    strncpy(st->cwd, "/", sizeof(st->cwd) - 1);

    proc->personality_data = st;
    return st;
}

void linux_destroy_process_state(struct bh_process *proc) {
    if (!proc) return;
    for (int i = 0; i < MAX_LINUX_PROCS; i++) {
        if (g_linux_proc_in_use[i] && g_linux_proc_pids[i] == proc->process_id) {
            linux_process_state_t *st = &g_linux_proc_states[i];
            for (int fd = 0; fd < LINUX_MAX_FDS; fd++) {
                if (st->fd_table[fd].type == LINUX_FD_TYPE_PIPE_READ) {
                    linux_pipe_close_read((linux_pipe_t *)st->fd_table[fd].priv_data);
                } else if (st->fd_table[fd].type == LINUX_FD_TYPE_PIPE_WRITE) {
                    linux_pipe_close_write((linux_pipe_t *)st->fd_table[fd].priv_data);
                } else if (st->fd_table[fd].type == LINUX_FD_TYPE_EVENTFD) {
                    linux_eventfd_unref((linux_eventfd_t *)st->fd_table[fd].priv_data);
                } else if (st->fd_table[fd].type == LINUX_FD_TYPE_TIMERFD) {
                    linux_timerfd_unref((linux_timerfd_t *)st->fd_table[fd].priv_data);
                } else if (st->fd_table[fd].type == LINUX_FD_TYPE_EPOLL) {
                    linux_epoll_unref((linux_epoll_t *)st->fd_table[fd].priv_data);
                } else if (st->fd_table[fd].type == LINUX_FD_TYPE_PROC) {
                    linux_procfs_file_unref((linux_procfs_file_t *)st->fd_table[fd].priv_data);
                }
            }
            g_linux_proc_in_use[i] = 0;
            g_linux_proc_pids[i] = 0;
            memset(&g_linux_proc_states[i], 0, sizeof(linux_process_state_t));
            break;
        }
    }
    proc->personality_data = NULL;
}

int linux_map_fd_to_capability(subsys_instance_t* env, int linux_fd, uint32_t cap, linux_fd_type_t type) {
    (void)env;
    bh_process_t *curr = sched_current_process();
    linux_process_state_t *st = linux_get_or_create_process_state(curr);
    if (!st || linux_fd < 0 || linux_fd >= LINUX_MAX_FDS) return -1;
    st->fd_table[linux_fd].type = type;
    st->fd_table[linux_fd].linux_fd = linux_fd;
    st->fd_table[linux_fd].backing_capability = cap;
    st->fd_table[linux_fd].open_flags = 0;
    st->fd_table[linux_fd].file_offset = 0;
    st->fd_table[linux_fd].ref_count = 1;
    return 0;
}

void linux_vfs_init(void) {
    /* Per-process tables initialize on demand upon process creation / lookup */
}
