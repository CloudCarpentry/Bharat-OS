#include "personality_ops.h"

#include "trap_frame_ops.h"

extern long syscall_dispatch(syscall_id_t id, uint64_t arg0, uint64_t arg1,
                      uint64_t arg2, uint64_t arg3, uint64_t arg4,
                      uint64_t arg5);

static long default_handle_syscall(kthread_t *thread, trap_frame_t *frame, const trap_info_t *info) {
    (void)thread;
    (void)info;

    return syscall_dispatch(
        trap_frame_get_syscall_no(frame),
        trap_frame_get_arg0(frame),
        trap_frame_get_arg1(frame),
        trap_frame_get_arg2(frame),
        trap_frame_get_arg3(frame),
        trap_frame_get_arg4(frame),
        trap_frame_get_arg5(frame)
    );
}

static int default_handle_user_fault(kthread_t *thread, trap_frame_t *frame, const trap_info_t *info) {
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
