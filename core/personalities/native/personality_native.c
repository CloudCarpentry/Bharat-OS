#include "personality_ops.h"
#include "trap_frame_ops.h"
#include "trap/syscall_regs.h"
#include "trap/syscall_context.h"
#include "bh_personality_registry.h"
#include "bh_personality.h"

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

static long native_normalize_syscall_return(long result) {
    // Native already returns kstatus_to_native_sysret internally in handlers
    // or through the gate.
    return result;
}

const personality_ops_t default_personality_ops = {
    .handle_syscall = default_handle_syscall,
    .handle_user_fault = default_handle_user_fault,
    .map_fault_to_signal = default_map_fault_to_signal,
    .normalize_syscall_return = native_normalize_syscall_return,
};

const personality_ops_t *personality_native_get_ops(void) {
    return &default_personality_ops;
}

void native_personality_init(void) {
    bh_personality_registry_register(BH_PERSONALITY_NATIVE, &default_personality_ops);

    // Validate the native syscall table at initialization
    if (bh_syscall_table_validate(&native_personality) != K_OK) {
        // In production, we might want a cleaner way to report this,
        // but for now, we should ensure the system doesn't boot with broken metadata.
        extern void kernel_panic(const char *msg);
        kernel_panic("Native syscall table validation failed");
    }
}
