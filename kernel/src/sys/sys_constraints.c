#include <bharat/constraints.h>
#include <core/constraint_validate.h>
#include <sched/sched.h>
#include <bharat/uapi/syscall/constraints.h>
#include <hal/hal.h>

void thread_set_constraints(int tid, const bh_exec_constraints_k_t *c) {
    kthread_t* thread = sched_find_thread_by_id(tid);
    if (thread) {
        thread->constraints = *c;
    }
}

int sys_thread_set_constraints(int tid, const bh_exec_constraints_t *user_c) {
    if (!user_c) return -1;

    bh_exec_constraints_k_t k = {0};

    k.flags = user_c->flags;
    k.cpu_mask = user_c->cpu_mask_hint;

    if (bh_validate_exec_constraints(&k) != BH_CONSTRAINT_OK)
        return -1;

    thread_set_constraints(tid, &k);

    // Reschedule the thread to enforce new constraints (per constraint-aware scheduling rules)
    kthread_t* thread = sched_find_thread_by_id(tid);
    if (thread && thread->state == THREAD_STATE_RUNNING) {
        // Trigger a reschedule on the thread's core to force the new constraint to be evaluated
        // For MVP, we can just yield the core if it's running locally, or rely on normal quantum expiration.
        // It's tricky to preempt remotely without ipi_reschedule exposing, so we'll just yield if local.
        uint32_t current_core = hal_cpu_get_id();
        if (thread->bound_core_id == current_core) {
            sched_reschedule();
        }
    }

    return 0;
}
