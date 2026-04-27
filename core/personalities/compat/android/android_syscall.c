#include "trap/syscall_context.h"
#include "kernel/status.h"
#include "bharat/personality/personality_interface.h"

// Android initially inherits/references Linux base conceptually
extern const bh_personality_syscall_table_t bh_linux_syscall_table;

// Future: Binder/Ashmem/Property handlers will be added here
static const bh_syscall_desc_t android_syscall_table[] = {
    // Placeholder for Android-specific extensions
};

const bh_personality_syscall_table_t android_personality_table = {
    .name = "android",
    .abi_version = 1,
    .max_syscall_nr = 0, // Fallback to Linux or fail closed
    .table = android_syscall_table
};

const bh_personality_syscall_table_t *personality_android_get_table(void) {
    // For now, return a minimal table that will result in ENOSYS for most things
    // but the gate can be extended to fall back to Linux.
    return &android_personality_table;
}
