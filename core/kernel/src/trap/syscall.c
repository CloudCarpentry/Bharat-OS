#include "trap/syscall_regs.h"
#include "trap/syscall_context.h"
#include "personality_ops.h"
#include "sched/sched.h"
#include "trap/syscall_status.h"

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
        // Fallback to error if no personality is set
        rc = (long)BH_ERR_NOT_SUPPORTED;
    }

    // Ensure the return value is set in the arch-specific register
    arch_trap_set_syscall_return(frame, (uintptr_t)rc);

    return rc;
}
