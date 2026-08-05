#include "sched/sched.h"
#include "sched_internal.h"
#include "hal/hal.h"
#include "hal/hal_timer.h"

void sched_edf_enqueue(sched_rq_t *rq, bh_thread_t *thread) {
    if (!thread) return;
    if (thread->absolute_deadline_ms == 0 && thread->rt_attr.deadline_ms > 0) {
        thread->absolute_deadline_ms = hal_timer_monotonic_ticks() + thread->rt_attr.deadline_ms;
    }
    sched_entity_t *entity = sched_find_entity_by_thread(thread);
    if (!entity) return;

    entity->absolute_deadline = thread->absolute_deadline_ms;

    struct rb_node **link = &rq->edf_runqueue.rb_node;
    struct rb_node *parent = NULL;
    uint64_t absolute_deadline = entity->absolute_deadline;

    while (*link) {
        parent = *link;
        sched_entity_t *entry = (sched_entity_t *)(void *)((char *)parent - offsetof(sched_entity_t, edf_node));

        if (absolute_deadline < entry->absolute_deadline) {
            link = &parent->rb_left;
        } else if (absolute_deadline > entry->absolute_deadline) {
            link = &parent->rb_right;
        } else {
            if (entity->priority > entry->priority) {
                link = &parent->rb_left;
            } else if (entity->priority < entry->priority) {
                link = &parent->rb_right;
            } else {
                if (entity->tid < entry->tid) {
                    link = &parent->rb_left;
                } else {
                    link = &parent->rb_right;
                }
            }
        }
    }

    rb_link_node(&entity->edf_node, parent, link);
    rb_insert_color(&entity->edf_node, &rq->edf_runqueue);
}

void sched_edf_dequeue(sched_rq_t *rq, bh_thread_t *thread) {
    sched_entity_t *entity = sched_find_entity_by_thread(thread);
    if (!entity) return;

    if (rq->edf_runqueue.rb_node == NULL) {
        return;
    }
    rb_erase(&entity->edf_node, &rq->edf_runqueue);
}

int sched_admission_edf(bh_thread_t *thread, uint64_t wcet_ms, uint64_t period_ms, uint64_t deadline_ms) {
    if (!thread || period_ms == 0 || wcet_ms == 0 || wcet_ms > deadline_ms || deadline_ms > period_ms) {
        return -1;
    }

    // Assign RT attributes
    thread->rt_attr.wcet_ms = wcet_ms;
    thread->rt_attr.period_ms = period_ms;
    thread->rt_attr.deadline_ms = deadline_ms;
    thread->absolute_deadline_ms = 0; // calculated on enqueue

    // Calculate bandwidth contribution (utilization * 1000)
    uint64_t bandwidth_needed = (wcet_ms * 1000) / period_ms;

    sched_rq_t *rq = sched_local_rq();

    spin_lock(&rq->lock);
    if (rq->rt_budget_total == 0) {
        rq->rt_budget_total = 1000; // 100% core utilization allowed for EDF tests
    }

    if (rq->rt_budget_used + bandwidth_needed > rq->rt_budget_total) {
        spin_unlock(&rq->lock);
        return -2; // Admission failed due to bandwidth limitations
    }

    rq->rt_budget_used += bandwidth_needed;
    spin_unlock(&rq->lock);

    return 0;
}

int sched_admission_rms(bh_thread_t *thread, uint64_t wcet_ms, uint64_t period_ms) {
    if (!thread || period_ms == 0 || wcet_ms > period_ms) {
        return -1;
    }

    // Rate Monotonic assigns static priorities based on period:
    // shorter period -> higher priority.
    // In our system, max priority is 31.
    // Let's create a simple mapping: e.g. base = 10, prio = min(31, 31 - period_ms/10 + constant)
    // For simplicity, we assign priority manually but ensure basic bounds.
    uint32_t new_prio = 10; // Default fallback
    if (period_ms <= 10) new_prio = 30;
    else if (period_ms <= 50) new_prio = 25;
    else if (period_ms <= 100) new_prio = 20;
    else new_prio = 15;

    // Optional bounds checking for n tasks: U <= n(2^(1/n) - 1). Simplified to 0.69 bound.
    uint64_t bandwidth_needed = (wcet_ms * 1000) / period_ms;

    sched_rq_t *rq = sched_local_rq();

    spin_lock(&rq->lock);
    if (rq->rt_budget_total == 0) {
        rq->rt_budget_total = 690; // RMS bound (~69%)
    }

    if (rq->rt_budget_used + bandwidth_needed > rq->rt_budget_total) {
        spin_unlock(&rq->lock);
        return -2; // Admission failed
    }

    rq->rt_budget_used += bandwidth_needed;
    spin_unlock(&rq->lock);

    thread->priority = new_prio;
    thread->base_priority = new_prio;
    thread->rt_attr.wcet_ms = wcet_ms;
    thread->rt_attr.period_ms = period_ms;
    thread->rt_attr.deadline_ms = period_ms; // implicit deadline

    return 0;
}

