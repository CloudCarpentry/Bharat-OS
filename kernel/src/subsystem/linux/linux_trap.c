#include "sched/sched.h"
#include "personality_ops.h"
#include "trap_types.h"
#include "trap.h"
#include "trap_frame_ops.h"

extern long linux_syscall_handler(long sysno, long arg1, long arg2, long arg3,
                                  long arg4, long arg5, long arg6);

static long linux_handle_syscall(struct bh_thread *thread, struct trap_frame *frame, const struct trap_info *info) {
    (void)thread;
    (void)info;
    return linux_syscall_handler(
        trap_frame_get_syscall_no(frame),
        trap_frame_get_arg0(frame),
        trap_frame_get_arg1(frame),
        trap_frame_get_arg2(frame),
        trap_frame_get_arg3(frame),
        trap_frame_get_arg4(frame),
        trap_frame_get_arg5(frame)
    );
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
