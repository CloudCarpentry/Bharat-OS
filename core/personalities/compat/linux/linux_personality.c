#include "linux_personality.h"
#include "bh_personality_registry.h"
#include "bh_personality.h"
#include "linux_errno.h"
#include <stddef.h>

extern const bh_personality_syscall_table_t bh_linux_syscall_table;
extern long bh_syscall_gate(trap_frame_t *frame, const trap_info_t *info);

const bh_personality_syscall_table_t *personality_linux_get_table(void) {
    return &bh_linux_syscall_table;
}

static long linux_handle_syscall(bh_thread_t *thread, trap_frame_t *frame, const trap_info_t *info) {
    (void)thread;
    return bh_syscall_gate(frame, info);
}

static int linux_handle_user_fault(bh_thread_t *thread, trap_frame_t *frame, const trap_info_t *info) {
    (void)thread; (void)frame; (void)info;
    return -1;
}

static int linux_map_fault_to_signal(const trap_info_t *info) {
    (void)info;
    return 11; // SIGSEGV
}

static long linux_normalize_syscall_return(long result) {
    // If result is in the range of kernel status codes (negative),
    // translate it to negative linux errno.
    if (result < 0 && result > -1000) { // Assuming kstatus codes are in this range
        return -linux_errno_from_bh_status((kstatus_t)result);
    }
    return result;
}

static const personality_ops_t linux_personality_ops = {
    .handle_syscall = linux_handle_syscall,
    .handle_user_fault = linux_handle_user_fault,
    .map_fault_to_signal = linux_map_fault_to_signal,
    .normalize_syscall_return = linux_normalize_syscall_return,
};

const personality_ops_t *personality_linux_get_ops(void) {
    return &linux_personality_ops;
}

void linux_personality_init(void) {
    bh_personality_registry_register(BH_PERSONALITY_LINUX, &linux_personality_ops);
}
