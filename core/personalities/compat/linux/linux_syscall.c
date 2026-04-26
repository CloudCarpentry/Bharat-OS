#include "linux_personality.h"
#include "linux_errno.h"
#include "linux_syscall_numbers_x86_64.h"
#include "trap/syscall_context.h"
#include "kernel/status.h"
#include "sched/sched.h"

static long linux_sys_getpid(bh_syscall_ctx_t *ctx) {
    if (!ctx || !ctx->process) return -LINUX_EINVAL;
    return (long)ctx->process->process_id;
}

static long linux_sys_gettid(bh_syscall_ctx_t *ctx) {
    if (!ctx || !ctx->thread) return -LINUX_EINVAL;
    return (long)ctx->thread->thread_id;
}

static long linux_sys_futex(bh_syscall_ctx_t *ctx) {
    (void)ctx;
    return -LINUX_ENOSYS;
}

extern long bh_sys_read(bh_syscall_ctx_t *ctx);
extern long bh_sys_write(bh_syscall_ctx_t *ctx);
extern long bh_sys_thread_exit(bh_syscall_ctx_t *ctx);

static const bh_syscall_desc_t linux_syscall_table_x86_64[] = {
    [LINUX_X86_64_SYS_READ]       = { LINUX_X86_64_SYS_READ, "read", 3, BH_SYSCALL_F_BLOCKING | BH_SYSCALL_F_USER_WRITE, 0, bh_sys_read },
    [LINUX_X86_64_SYS_WRITE]      = { LINUX_X86_64_SYS_WRITE, "write", 3, BH_SYSCALL_F_BLOCKING | BH_SYSCALL_F_USER_READ, 0, bh_sys_write },
    [LINUX_X86_64_SYS_GETPID]     = { LINUX_X86_64_SYS_GETPID, "getpid", 0, BH_SYSCALL_F_FAST, 0, linux_sys_getpid },
    [LINUX_X86_64_SYS_EXIT]       = { LINUX_X86_64_SYS_EXIT, "exit", 1, 0, 0, bh_sys_thread_exit },
    [LINUX_X86_64_SYS_EXIT_GROUP] = { LINUX_X86_64_SYS_EXIT_GROUP, "exit_group", 1, 0, 0, bh_sys_thread_exit },
};

const bh_personality_syscall_table_t bh_linux_syscall_table = {
    .name = "linux",
    .abi_version = 1,
    .max_syscall_nr = 231,
    .table = linux_syscall_table_x86_64
};
