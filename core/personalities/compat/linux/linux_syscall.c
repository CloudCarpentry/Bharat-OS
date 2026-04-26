#include "trap/syscall_context.h"
#include "kernel/status.h"

// Linux-specific syscall numbers (x86_64 example set)
#define LINUX_SYS_READ          0
#define LINUX_SYS_WRITE         1
#define LINUX_SYS_GETPID        39
#define LINUX_SYS_GETTID        186
#define LINUX_SYS_FUTEX         202
#define LINUX_SYS_EXIT          60
#define LINUX_SYS_EXIT_GROUP    231

extern long bh_sys_read(bh_syscall_ctx_t *ctx);
extern long bh_sys_write(bh_syscall_ctx_t *ctx);
extern long bh_sys_thread_exit(bh_syscall_ctx_t *ctx);

#include "sched/sched.h"

// Linux errno mappings (subset for stubs)
#define LINUX_ENOSYS 38

static long linux_sys_getpid(bh_syscall_ctx_t *ctx) {
    if (!ctx || !ctx->process) return -1;
    return (long)ctx->process->process_id;
}

static long linux_sys_gettid(bh_syscall_ctx_t *ctx) {
    if (!ctx || !ctx->thread) return -1;
    return (long)ctx->thread->thread_id;
}

static long linux_sys_futex(bh_syscall_ctx_t *ctx) {
    (void)ctx;
    return -LINUX_ENOSYS; // Do not fake success
}

static const bh_syscall_desc_t linux_syscall_table[] = {
    [LINUX_SYS_READ]       = { LINUX_SYS_READ, "read", 3, BH_SYSCALL_F_BLOCKING | BH_SYSCALL_F_USER_WRITE, 0, bh_sys_read },
    [LINUX_SYS_WRITE]      = { LINUX_SYS_WRITE, "write", 3, BH_SYSCALL_F_BLOCKING | BH_SYSCALL_F_USER_READ, 0, bh_sys_write },
    [LINUX_SYS_GETPID]     = { LINUX_SYS_GETPID, "getpid", 0, BH_SYSCALL_F_FAST, 0, linux_sys_getpid },
    [LINUX_SYS_GETTID]     = { LINUX_SYS_GETTID, "gettid", 0, BH_SYSCALL_F_FAST, 0, linux_sys_gettid },
    [LINUX_SYS_FUTEX]      = { LINUX_SYS_FUTEX, "futex", 6, BH_SYSCALL_F_BLOCKING, 0, linux_sys_futex },
    [LINUX_SYS_EXIT]       = { LINUX_SYS_EXIT, "exit", 1, 0, 0, bh_sys_thread_exit },
    [LINUX_SYS_EXIT_GROUP] = { LINUX_SYS_EXIT_GROUP, "exit_group", 1, 0, 0, bh_sys_thread_exit },
};

const bh_personality_syscall_table_t bh_linux_syscall_table = {
    .name = "linux",
    .abi_version = 1,
    .max_syscall_nr = 231,
    .table = linux_syscall_table
};
