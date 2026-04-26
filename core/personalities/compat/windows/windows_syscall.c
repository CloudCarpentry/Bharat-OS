#include "trap/syscall_context.h"
#include "kernel/status.h"
#include "bharat/personality/personality_interface.h"

// Windows NT-style table scaffold
#define STATUS_NOT_IMPLEMENTED ((long)0xC0000002L)

static long win_sys_unsupported(bh_syscall_ctx_t *ctx) {
    (void)ctx;
    return STATUS_NOT_IMPLEMENTED;
}

static const bh_syscall_desc_t windows_syscall_table[] = {
    [0] = { 0, "NtUnsupported", 0, 0, 0, win_sys_unsupported },
};

const bh_personality_syscall_table_t windows_personality_table = {
    .name = "windows",
    .abi_version = 1,
    .max_syscall_nr = 0,
    .table = windows_syscall_table
};

const bh_personality_syscall_table_t *personality_windows_get_table(void) {
    return &windows_personality_table;
}
