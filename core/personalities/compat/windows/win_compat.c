#include "win_compat.h"
#include "trap/syscall_context.h"
#include "personality_ops.h"

extern long bh_syscall_gate(trap_frame_t *frame, const trap_info_t *info);

static long windows_handle_syscall(bh_thread_t *thread, trap_frame_t *frame, const trap_info_t *info) {
    (void)thread;
    return bh_syscall_gate(frame, info);
}

static int windows_handle_user_fault(bh_thread_t *thread, trap_frame_t *frame, const trap_info_t *info) {
    (void)thread; (void)frame; (void)info;
    return -1;
}

static int windows_map_fault_to_signal(const trap_info_t *info) {
    (void)info;
    return 11; // SIGSEGV equivalent/dummy
}

static const personality_ops_t windows_personality_ops = {
    .handle_syscall = windows_handle_syscall,
    .handle_user_fault = windows_handle_user_fault,
    .map_fault_to_signal = windows_map_fault_to_signal,
};

int winnt_subsys_init(subsys_instance_t* env) {
    if (!env) {
        return -1;
    }

    if (env->type != SUBSYS_TYPE_WINDOWS) {
        return -2;
    }

    /* Stub personality: reserved for future NT syscall/PE emulation. */
    env->memory_limit_mb = (env->memory_limit_mb == 0U) ? 2048U : env->memory_limit_mb;
    return 0;
}

const personality_ops_t *personality_windows_get_ops(void) {
    return &windows_personality_ops;
}
