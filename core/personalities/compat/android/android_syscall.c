#include "trap/syscall_context.h"
#include "kernel/status.h"
#include "bharat/personality/personality_interface.h"
#include "linux_personality.h"

/**
 * @brief Android Personality Syscall Dispatch Table
 *
 * Android Bionic libc invokes standard Linux syscall ABI numbers across all
 * supported ISAs (x86_64, arm64, riscv64, arm32, riscv32).
 * Android-specific device extensions (/dev/binder, /dev/ashmem) operate
 * through standard ioctl/mmap file descriptors multiplexed over the
 * capability-backed Android personality runtime.
 */
const bh_personality_syscall_table_t *personality_android_get_table(void) {
    return personality_linux_get_table();
}
