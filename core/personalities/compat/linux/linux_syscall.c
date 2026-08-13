#include "linux_personality.h"
#include "linux_errno.h"
#include "linux_syscall_numbers_x86_64.h"
#include "trap/syscall_context.h"
#include "kernel/status.h"
#include "sched/sched.h"
#include "syscall/usercopy.h"
#include "trap/syscall_status.h"
#include "time/ktime.h"

#define LINUX_CLOCK_MONOTONIC 1U
#define LINUX_NSEC_PER_SEC UINT64_C(1000000000)

typedef struct {
    int64_t tv_sec;
    int64_t tv_nsec;
} linux_timespec_t;

static bh_operation_result_t linux_sys_getpid(bh_syscall_ctx_t *ctx) {
    if (!ctx || !ctx->process) return bh_op_result_kstatus(K_ERR_INVALID_ARG);
    return bh_op_result_value((long)ctx->process->process_id);
}

static bh_operation_result_t linux_sys_gettid(bh_syscall_ctx_t *ctx) {
    if (!ctx || !ctx->thread) return bh_op_result_kstatus(K_ERR_INVALID_ARG);
    return bh_op_result_value((long)ctx->thread->thread_id);
}

static bh_operation_result_t linux_sys_futex(bh_syscall_ctx_t *ctx) {
    (void)ctx;
    return bh_op_result_kstatus(K_ERR_UNSUPPORTED);
}

static bh_operation_result_t linux_sys_clock_gettime(bh_syscall_ctx_t *ctx) {
    uint64_t now_ns;
    linux_timespec_t value;
    bh_status_t status;

    if ((uint32_t)ctx->regs.arg[0] != LINUX_CLOCK_MONOTONIC) {
        return bh_op_result_kstatus(K_ERR_UNSUPPORTED);
    }
    now_ns = bh_ktime_now();
    value.tv_sec = (int64_t)(now_ns / LINUX_NSEC_PER_SEC);
    value.tv_nsec = (int64_t)(now_ns % LINUX_NSEC_PER_SEC);
    status = bh_copy_to_user((void *)ctx->regs.arg[1], &value, sizeof(value));
    return bh_op_result_kstatus(bh_status_to_kstatus(status));
}

extern bh_operation_result_t bh_sys_read(bh_syscall_ctx_t *ctx);
extern bh_operation_result_t bh_sys_write(bh_syscall_ctx_t *ctx);
extern bh_operation_result_t bh_sys_thread_exit(bh_syscall_ctx_t *ctx);

static const bh_syscall_meta_t linux_syscall_table_x86_64[] = {
    [LINUX_X86_64_SYS_READ]       = { .nr = LINUX_X86_64_SYS_READ, .name = "read", .class_id = BH_SYS_CLASS_IO, .arg_count = 3, .flags = BH_SYSCALL_F_BLOCKING | BH_SYSCALL_F_USER_WRITE, .handler = bh_sys_read },
    [LINUX_X86_64_SYS_WRITE]      = { .nr = LINUX_X86_64_SYS_WRITE, .name = "write", .class_id = BH_SYS_CLASS_IO, .arg_count = 3, .flags = BH_SYSCALL_F_BLOCKING | BH_SYSCALL_F_USER_READ, .handler = bh_sys_write },
    [LINUX_X86_64_SYS_GETPID]     = { .nr = LINUX_X86_64_SYS_GETPID, .name = "getpid", .class_id = BH_SYS_CLASS_PROCESS, .arg_count = 0, .flags = BH_SYSCALL_F_FAST, .handler = linux_sys_getpid },
    [LINUX_X86_64_SYS_EXIT]       = { .nr = LINUX_X86_64_SYS_EXIT, .name = "exit", .class_id = BH_SYS_CLASS_PROCESS, .arg_count = 1, .handler = bh_sys_thread_exit },
    [LINUX_X86_64_SYS_EXIT_GROUP] = { .nr = LINUX_X86_64_SYS_EXIT_GROUP, .name = "exit_group", .class_id = BH_SYS_CLASS_PROCESS, .arg_count = 1, .handler = bh_sys_thread_exit },
    [LINUX_X86_64_SYS_CLOCK_GETTIME] = { .nr = LINUX_X86_64_SYS_CLOCK_GETTIME, .name = "clock_gettime", .class_id = BH_SYS_CLASS_SYSTEM, .arg_count = 2, .flags = BH_SYSCALL_F_USER_WRITE, .handler = linux_sys_clock_gettime },
};

const bh_personality_syscall_table_t bh_linux_syscall_table = {
    .name = "linux",
    .abi_version = 1,
    .entry_count = 232,
    .table = linux_syscall_table_x86_64
};
