#include "sched/sched.h"
#include "personality_ops.h"
#include "trap_types.h"
#include "trap.h"
#include "trap/syscall_regs.h"

extern long bh_syscall_gate(trap_frame_t *frame, const trap_info_t *info);

static long linux_handle_syscall(struct bh_thread *thread, struct trap_frame *frame, const struct trap_info *info) {
    (void)thread;
    // Dispatch via common secure syscall gate
    return bh_syscall_gate(frame, info);
}

static int linux_handle_user_fault(struct bh_thread *thread, struct trap_frame *frame, const struct trap_info *info) {
    (void)thread;
    (void)frame;
    (void)info;
    return -1; // Default
}

static int linux_map_fault_to_signal(const struct trap_info *info) {
    (void)info;
    return 11; // SIGSEGV default
}

const personality_ops_t linux_personality_ops = {
    .handle_syscall = linux_handle_syscall,
    .handle_user_fault = linux_handle_user_fault,
    .map_fault_to_signal = linux_map_fault_to_signal,
};
