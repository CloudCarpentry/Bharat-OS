#include "trap/syscall_regs.h"
#include "trap/syscall_context.h"
#include "personality_ops.h"
#include "sched/sched.h"
#include "kernel/status.h"

/**
 * trap_dispatch_syscall: Entry point from generic trap handler.
 * Routes execution through the thread's personality.
 *
 * CONTRACT: This function is the SOLE OWNER of arch_trap_set_syscall_return().
 * All personality handlers and the common gate must return a normalized long.
 */
long trap_dispatch_syscall(trap_frame_t *frame, const trap_info_t *info) {
    bh_thread_t *current = sched_current_thread();
    long rc;

    if (current && current->process && current->process->personality_ops && current->process->personality_ops->handle_syscall) {
        rc = current->process->personality_ops->handle_syscall(current, frame, info);
    } else {
        // Fallback to native or error if no personality is set
        rc = kstatus_to_sysret(K_ERR_UNSUPPORTED);
    }

    // Ensure the return value is set in the arch-specific register
    arch_trap_set_syscall_return(frame, (uintptr_t)rc);

    return rc;
}

// Legacy stub, to be removed
long syscall_dispatch(syscall_id_t id, uintptr_t arg0, uintptr_t arg1,
                      uintptr_t arg2, uintptr_t arg3, uintptr_t arg4,
                      uintptr_t arg5) {
    (void)id; (void)arg0; (void)arg1; (void)arg2; (void)arg3; (void)arg4; (void)arg5;
    return kstatus_to_sysret(K_ERR_UNSUPPORTED);
}
