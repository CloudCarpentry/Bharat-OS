#include <bharat/constraints.h>
#include <core/constraint_validate.h>
#include <sched/sched.h>
#include <bharat/uapi/syscall/constraints.h>
#include <hal/hal.h>

static void bh_copy_exec_constraints_uapi_to_kern(
    bh_exec_constraints_k_t *dst,
    const bh_exec_constraints_t *src) {
    dst->flags = src->flags;
    dst->cpu_mask = src->cpu_mask_hint;
    dst->latency_class = (uint16_t)src->latency_target_us;
    dst->energy_class = (uint16_t)src->energy_budget_uw;
}

static void bh_copy_exec_constraints_kern_to_uapi(
    bh_exec_constraints_t *dst,
    const bh_exec_constraints_k_t *src) {
    dst->flags = src->flags;
    dst->priority_class = 0;
    dst->latency_target_us = src->latency_class;
    dst->energy_budget_uw = src->energy_class;
    dst->memory_budget_kb = 0;
    dst->isolation_domain = 0;
    dst->cpu_mask_hint = src->cpu_mask;
}

static int thread_set_constraints(int tid, const bh_exec_constraints_k_t *c) {
    return sched_set_constraints(tid, c);
}

static int thread_get_constraints(int tid, bh_exec_constraints_k_t *out_c) {
    return sched_get_constraints(tid, out_c);
}

int sys_thread_set_constraints(int tid, const bh_exec_constraints_t *user_c) {
    if (!user_c) return -1;

    bh_exec_constraints_k_t k = {0};
    bh_copy_exec_constraints_uapi_to_kern(&k, user_c);

    if (bh_validate_exec_constraints(&k) != BH_CONSTRAINT_OK)
        return -1;

    if (thread_set_constraints(tid, &k) != 0)
        return -1;

    // Reschedule the thread to enforce new constraints (per constraint-aware scheduling rules)
    sched_thread_snapshot_t snap;
    if (sched_get_thread_snapshot(tid, &snap) == K_OK) {
        if (snap.state == THREAD_STATE_RUNNING) {
            uint32_t current_core = hal_cpu_get_id();
            if (snap.bound_core_id == current_core) {
                sched_reschedule();
            }
        }
    }

    return 0;
}

int sys_thread_get_constraints(int tid, bh_exec_constraints_t *out_c) {
    if (!out_c) return -1;

    bh_exec_constraints_k_t k = {0};
    if (thread_get_constraints(tid, &k) != 0)
        return -1;

    bh_copy_exec_constraints_kern_to_uapi(out_c, &k);
    return 0;
}
