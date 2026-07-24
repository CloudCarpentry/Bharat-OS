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

static const bh_syscall_meta_t linux_syscall_table_x86_64[] = {
    [LINUX_X86_64_SYS_READ]       = { .nr = LINUX_X86_64_SYS_READ, .name = "read", .class_id = BH_SYS_CLASS_IO, .arg_count = 3, .flags = BH_SYSCALL_F_BLOCKING | BH_SYSCALL_F_USER_WRITE, .handler = bh_sys_read },
    [LINUX_X86_64_SYS_WRITE]      = { .nr = LINUX_X86_64_SYS_WRITE, .name = "write", .class_id = BH_SYS_CLASS_IO, .arg_count = 3, .flags = BH_SYSCALL_F_BLOCKING | BH_SYSCALL_F_USER_READ, .handler = bh_sys_write },
    [LINUX_X86_64_SYS_GETPID]     = { .nr = LINUX_X86_64_SYS_GETPID, .name = "getpid", .class_id = BH_SYS_CLASS_PROCESS, .arg_count = 0, .flags = BH_SYSCALL_F_FAST, .handler = linux_sys_getpid },
    [LINUX_X86_64_SYS_EXIT]       = { .nr = LINUX_X86_64_SYS_EXIT, .name = "exit", .class_id = BH_SYS_CLASS_PROCESS, .arg_count = 1, .handler = bh_sys_thread_exit },
    [LINUX_X86_64_SYS_EXIT_GROUP] = { .nr = LINUX_X86_64_SYS_EXIT_GROUP, .name = "exit_group", .class_id = BH_SYS_CLASS_PROCESS, .arg_count = 1, .handler = bh_sys_thread_exit },
};

const bh_personality_syscall_table_t bh_linux_syscall_table = {
    .name = "linux",
    .abi_version = 1,
    .entry_count = 232,
    .table = linux_syscall_table_x86_64
};
