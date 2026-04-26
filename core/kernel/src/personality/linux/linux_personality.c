#include "trap/syscall_context.h"
#include "kernel/status.h"

static long linux_sys_getpid(bh_syscall_ctx_t *ctx) {
    (void)ctx;
    // Return dummy PID for now
    return 1000;
}

static const bh_syscall_desc_t linux_syscall_table[] = {
    [39] = { 39, "getpid", 0, BH_SYSCALL_F_FAST, 0, linux_sys_getpid },
};

static const bh_personality_syscall_table_t linux_personality = {
    .name = "linux",
    .abi_version = 1,
    .max_syscall_nr = 512,
    .table = linux_syscall_table
};

const bh_personality_syscall_table_t *linux_personality_get_table(void) {
    return &linux_personality;
}
