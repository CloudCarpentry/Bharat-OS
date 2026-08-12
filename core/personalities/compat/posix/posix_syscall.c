#include "posix_personality.h"
#include "posix_errno.h"
#include "posix_syscall_numbers.h"
#include "trap/syscall_context.h"
#include "kernel/status.h"
#include "sched/sched.h"

static long posix_sys_getpid(bh_syscall_ctx_t *ctx) {
    if (!ctx || !ctx->process) return -POSIX_EINVAL;
    return (long)ctx->process->process_id;
}

extern long bh_sys_read(bh_syscall_ctx_t *ctx);
extern long bh_sys_write(bh_syscall_ctx_t *ctx);
extern long bh_sys_thread_exit(bh_syscall_ctx_t *ctx);

static const bh_syscall_meta_t posix_syscall_table_base[] = {
    [POSIX_SYS_READ]       = { .nr = POSIX_SYS_READ, .name = "read", .class_id = BH_SYS_CLASS_IO, .arg_count = 3, .flags = BH_SYSCALL_F_BLOCKING | BH_SYSCALL_F_USER_WRITE, .handler = bh_sys_read },
    [POSIX_SYS_WRITE]      = { .nr = POSIX_SYS_WRITE, .name = "write", .class_id = BH_SYS_CLASS_IO, .arg_count = 3, .flags = BH_SYSCALL_F_BLOCKING | BH_SYSCALL_F_USER_READ, .handler = bh_sys_write },
    [POSIX_SYS_GETPID]     = { .nr = POSIX_SYS_GETPID, .name = "getpid", .class_id = BH_SYS_CLASS_PROCESS, .arg_count = 0, .flags = BH_SYSCALL_F_FAST, .handler = posix_sys_getpid },
    [POSIX_SYS_EXIT]       = { .nr = POSIX_SYS_EXIT, .name = "exit", .class_id = BH_SYS_CLASS_PROCESS, .arg_count = 1, .handler = bh_sys_thread_exit },
    [POSIX_SYS_EXIT_GROUP] = { .nr = POSIX_SYS_EXIT_GROUP, .name = "exit_group", .class_id = BH_SYS_CLASS_PROCESS, .arg_count = 1, .handler = bh_sys_thread_exit },
};

const bh_personality_syscall_table_t bh_posix_syscall_table = {
    .name = "posix",
    .abi_version = 1,
    .entry_count = 232,
    .table = posix_syscall_table_base
};
