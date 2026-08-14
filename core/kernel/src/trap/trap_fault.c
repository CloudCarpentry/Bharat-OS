#include "trap_api.h"
#include "sched/sched.h"
#include "panic.h"
#include "fault_diag.h"
#include "exception_table.h"

int (*g_test_fault_hook)(trap_frame_t *frame, const trap_info_t *info) = NULL;

kstatus_t trap_handle_fault(trap_frame_t *frame, const trap_info_t *info) {
    bh_thread_t *t = sched_current_thread();

    if (g_test_fault_hook) {
        int ret = g_test_fault_hook(frame, info);
        if (ret == 0) return K_OK;
    }

    if (info->origin == TRAP_ORIGIN_KERNEL) {
        // Attempt exception table recovery before panicking on kernel exceptions
        if (trap_try_exception_fixup(frame, info->fault_addr, info->arch_code)) {
            return K_OK; // Recovered successfully
        }

        panic_context_t pctx = {
            .message = "Kernel exception",
            .cause_str = "unhandled_kernel_trap",
            .cause_code = info->arch_code,
            .ip = info->ip,
            .sp = info->sp,
            .trap_frame = frame
        };
        kernel_panic_ex(&pctx);
        return K_ERR_FAULT; // -EFAULT
    }

    if (t && t->process && t->process->personality_ops &&
        t->process->personality_ops->handle_user_fault) {
        return t->process->personality_ops->handle_user_fault(t, frame, info);
    }

    if (t) {
        thread_raise_fault(t, THREAD_FAULT_SEGV);
    } else {
        bh_thread_yield();
    }

    return K_ERR_FAULT;
}
