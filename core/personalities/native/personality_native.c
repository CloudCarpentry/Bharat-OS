#include "personality_ops.h"
#include "trap_frame_ops.h"
#include "trap/syscall_regs.h"
#include "trap/syscall_context.h"

// bh_syscall_gate is now the common entry point
extern long bh_syscall_gate(trap_frame_t *frame, const trap_info_t *info);
extern const bh_personality_syscall_table_t native_personality;

const bh_personality_syscall_table_t *personality_native_get_table(void) {
    return &native_personality;
}

static long default_handle_syscall(bh_thread_t *thread, trap_frame_t *frame, const trap_info_t *info) {
    (void)thread;
    // Native personality simply calls the common gate
    return bh_syscall_gate(frame, info);
}

static int default_handle_user_fault(bh_thread_t *thread, trap_frame_t *frame, const trap_info_t *info) {
    (void)thread;
    (void)frame;
    (void)info;
    return -1; // General failure
}

static int default_map_fault_to_signal(const trap_info_t *info) {
    (void)info;
    return 11; // SIGSEGV
}

const personality_ops_t default_personality_ops = {
    .handle_syscall = default_handle_syscall,
    .handle_user_fault = default_handle_user_fault,
    .map_fault_to_signal = default_map_fault_to_signal,
};

const personality_ops_t *personality_native_get_ops(void) {
    return &default_personality_ops;
}
