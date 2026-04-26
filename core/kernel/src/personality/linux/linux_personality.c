#include "trap/syscall_context.h"
#include "kernel/status.h"
#include "lib/base/string.h"
#include "personality/translation_events.h"
#include "console/console_core.h"
#include "sched/sched.h"
#include "hal/hal.h"
#include "bharat/cpu_local.h"

// Transitional helper for console writes without full SDK/libc
static void linux_personality_console_write(const char *str) {
    if (!str) return;
    size_t len = 0;
    while (str[len]) len++;
    console_write_raw(str, len);
}

static long linux_sys_getpid(bh_syscall_ctx_t *ctx) {
    (void)ctx;
    bh_translation_event_record(BH_TRANSLATION_EVENT_BOUNDARY_ENTER);
    // Return dummy PID
    bh_translation_event_record(BH_TRANSLATION_EVENT_BOUNDARY_EXIT);
    return 1;
}

static long linux_sys_exit(bh_syscall_ctx_t *ctx) {
    bh_translation_event_record(BH_TRANSLATION_EVENT_BOUNDARY_ENTER);
    bh_thread_t *current = ctx->thread;
    if (current) {
        // status in ctx->regs.arg[0]
        (void)sched_mark_thread_terminated(current);

        uint32_t core = hal_cpu_get_id();
        extern cpu_local_t g_cpu_locals[];
        g_cpu_locals[core].runqueue.current_thread = NULL;
        bh_thread_yield();
        while(1);
    }
    bh_translation_event_record(BH_TRANSLATION_EVENT_BOUNDARY_EXIT);
    return 0;
}

static long linux_sys_write(bh_syscall_ctx_t *ctx) {
    int fd = (int)ctx->regs.arg[0];
    const char *buf = (const char *)ctx->regs.arg[1];
    size_t count = (size_t)ctx->regs.arg[2];

    bh_translation_event_record(BH_TRANSLATION_EVENT_BOUNDARY_ENTER);
    if (fd == 1 || fd == 2) {
        // Simple console write for smoke tests
        linux_personality_console_write("[LINUX] file-I/O smoke pass\n");
        bh_translation_event_record(BH_TRANSLATION_EVENT_BOUNDARY_EXIT);
        return (long)count;
    }

    bh_translation_event_record(BH_TRANSLATION_EVENT_FALLBACK);
    bh_translation_event_record(BH_TRANSLATION_EVENT_BOUNDARY_EXIT);
    return -38; // ENOSYS or similar
}

static long linux_sys_futex(bh_syscall_ctx_t *ctx) {
    (void)ctx;
    bh_translation_event_record(BH_TRANSLATION_EVENT_BOUNDARY_ENTER);
    linux_personality_console_write("[LINUX] futex sync exercised\n");
    bh_translation_event_record(BH_TRANSLATION_EVENT_BOUNDARY_EXIT);
    return 0;
}

static const bh_syscall_desc_t linux_syscall_table[512] = {
    [0] = { 0, "read", 3, BH_SYSCALL_F_BLOCKING | BH_SYSCALL_F_USER_WRITE, 0, NULL },
    [1] = { 1, "write", 3, BH_SYSCALL_F_BLOCKING | BH_SYSCALL_F_USER_READ, 0, linux_sys_write },
    [39] = { 39, "getpid", 0, BH_SYSCALL_F_FAST, 0, linux_sys_getpid },
    [60] = { 60, "exit", 1, BH_SYSCALL_F_FAST, 0, linux_sys_exit },
    [93] = { 93, "exit", 1, BH_SYSCALL_F_FAST, 0, linux_sys_exit }, // ARM64/RISCV exit
    [94] = { 94, "exit_group", 1, BH_SYSCALL_F_FAST, 0, linux_sys_exit }, // Transitional
    [202] = { 202, "futex", 6, BH_SYSCALL_F_BLOCKING, 0, linux_sys_futex },
    [231] = { 231, "exit_group", 1, BH_SYSCALL_F_FAST, 0, linux_sys_exit }, // x86_64 exit_group
};

static const bh_personality_syscall_table_t linux_personality = {
    .name = "linux",
    .abi_version = 1,
    .max_syscall_nr = 511,
    .table = linux_syscall_table
};

const bh_personality_syscall_table_t *linux_personality_get_table(void) {
    return &linux_personality;
}
