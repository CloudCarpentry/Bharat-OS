#include "posix_personality.h"
#include "bh_personality_registry.h"
#include "bh_personality.h"
#include "posix_errno.h"
#include <stddef.h>

extern const bh_personality_syscall_table_t bh_posix_syscall_table;
extern long bh_syscall_gate(trap_frame_t *frame, const trap_info_t *info);

const bh_personality_syscall_table_t *personality_posix_get_table(void) {
    return &bh_posix_syscall_table;
}

static long posix_handle_syscall(bh_thread_t *thread, trap_frame_t *frame, const trap_info_t *info) {
    (void)thread;
    return bh_syscall_gate(frame, info);
}

static int posix_handle_user_fault(bh_thread_t *thread, trap_frame_t *frame, const trap_info_t *info) {
    (void)thread; (void)frame; (void)info;
    return -1;
}

static int posix_map_fault_to_signal(const trap_info_t *info) {
    (void)info;
    return 11; // SIGSEGV
}

static long posix_normalize_syscall_return(long result) {
    // If result is in the range of kernel status codes (negative),
    // translate it to negative posix errno.
    if (result < 0 && result > -1000) { // Assuming kstatus codes are in this range
        return -result; // Simplified mapping for now, ideally translate BH_STATUS to POSIX_ERRNO
    }
    return result;
}

static const personality_ops_t posix_personality_ops = {
    .handle_syscall = posix_handle_syscall,
    .handle_user_fault = posix_handle_user_fault,
    .map_fault_to_signal = posix_map_fault_to_signal,
    .normalize_syscall_return = posix_normalize_syscall_return,
};

const personality_ops_t *personality_posix_get_ops(void) {
    return &posix_personality_ops;
}

void posix_personality_init(void) {
    bh_personality_registry_register(BH_PERSONALITY_POSIX_LITE, &posix_personality_ops);
}
