#include "android_personality.h"
#include "bh_personality_registry.h"
#include "bh_personality.h"
#include <stddef.h>

extern const personality_ops_t *personality_linux_get_ops(void);
extern const bh_personality_syscall_table_t *personality_android_get_table(void);
extern long bh_syscall_gate(trap_frame_t *frame, const trap_info_t *info);

static long android_handle_syscall(bh_thread_t *thread, trap_frame_t *frame, const trap_info_t *info) {
    (void)thread;
    return bh_syscall_gate(frame, info); // Route to common gate which handles fallback internally
}

const personality_ops_t *personality_android_get_ops(void) {
    // We override handle_syscall to demonstrate composition
    // but fallbacks must be populated. Reusing linux ops is safer for missing items.
    static personality_ops_t combined_ops;
    const personality_ops_t *linux_ops = personality_linux_get_ops();

    combined_ops.handle_syscall = android_handle_syscall;
    combined_ops.handle_user_fault = linux_ops->handle_user_fault;
    combined_ops.map_fault_to_signal = linux_ops->map_fault_to_signal;
    combined_ops.normalize_syscall_return = linux_ops->normalize_syscall_return;

    return &combined_ops;
}

void android_personality_init(void) {
    bh_personality_registry_register(BH_PERSONALITY_ANDROID, personality_android_get_ops());
}
