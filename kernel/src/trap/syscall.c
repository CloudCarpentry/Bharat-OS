#include "trap_api.h"
#include "trap_frame_ops.h"
#include "personality_ops.h"
#include "sched/sched.h"
#include "fault_diag.h"

#define ENOSYS 38

long trap_dispatch_syscall(trap_frame_t *frame, const trap_info_t *info) {
    bh_thread_t *t = sched_current_thread();
    if (!t || !t->process || !t->process->personality_ops ||
        !t->process->personality_ops->handle_syscall) {
        return -ENOSYS;
    }

    fault_diag_record_syscall(trap_frame_get_syscall_no(frame));

    long rc = t->process->personality_ops->handle_syscall(t, frame, info);

    trap_frame_set_retval(frame, (uintptr_t)rc);

    return rc;
}
