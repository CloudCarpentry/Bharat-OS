#include "trap/syscall_context.h"
#include "kernel/status.h"

static const bh_personality_syscall_table_t windows_personality = {
    .name = "windows",
    .abi_version = 1,
    .max_syscall_nr = 1024,
    .table = NULL
};

const bh_personality_syscall_table_t *windows_personality_get_table(void) {
    return &windows_personality;
}
