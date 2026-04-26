#include "trap/syscall_context.h"
#include "kernel/status.h"

extern const bh_personality_syscall_table_t *linux_personality_get_table(void);

static const bh_personality_syscall_table_t android_personality = {
    .name = "android",
    .abi_version = 1,
    .max_syscall_nr = 512,
    .table = NULL // Will reuse/extend linux table in future
};

const bh_personality_syscall_table_t *android_personality_get_table(void) {
    // For now reuse linux
    return linux_personality_get_table();
}
