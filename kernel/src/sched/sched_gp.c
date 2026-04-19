#include "sched/sched.h"
#include "sched_internal.h"

void sched_cfs_enqueue(sched_rq_t *rq, kthread_t *thread) {
    struct rb_node **link = &rq->cfs_runqueue.rb_node;
    struct rb_node *parent = NULL;
    int64_t vruntime = thread->vruntime;
    int leftmost = 1;

    // Wakeup preemption edge case: bound the negative drift
    if (vruntime < rq->min_vruntime) {
        vruntime = rq->min_vruntime;
        thread->vruntime = vruntime;
    }

    while (*link) {
        parent = *link;
        kthread_t *entry = (kthread_t *)(void *)((char *)parent - offsetof(kthread_t, cfs_node));

        if (vruntime < entry->vruntime) {
            link = &parent->rb_left;
        } else {
            link = &parent->rb_right;
            leftmost = 0;
        }
    }

    rb_link_node(&thread->cfs_node, parent, link);
    rb_insert_color(&thread->cfs_node, &rq->cfs_runqueue);

    // Update min_vruntime if this is the new leftmost node
    if (leftmost) {
        rq->min_vruntime = vruntime;
    }
}

void sched_cfs_dequeue(sched_rq_t *rq, kthread_t *thread) {
    if (rq->cfs_runqueue.rb_node == NULL) {
        return;
    }

    // Is it the leftmost?
    int leftmost = (rb_first(&rq->cfs_runqueue) == &thread->cfs_node);

    rb_erase(&thread->cfs_node, &rq->cfs_runqueue);

    if (leftmost) {
        struct rb_node *first = rb_first(&rq->cfs_runqueue);
        if (first) {
            kthread_t *next = (kthread_t *)(void *)((char *)first - offsetof(kthread_t, cfs_node));
            rq->min_vruntime = next->vruntime;
        }
    }
}

void sched_cfs_update_vruntime(sched_rq_t *rq, kthread_t *thread, uint64_t delta_exec) {
    if (delta_exec == 0) return;

    // Validate monotonic growth
    int64_t prev_vruntime = thread->vruntime;

    // vruntime += (delta_exec * NICE_0_WEIGHT) / weight
    uint32_t weight = thread->weight > 0 ? thread->weight : 1;
    uint64_t delta_vruntime = (delta_exec * CFS_NICE_0_WEIGHT) / weight;

    thread->vruntime += delta_vruntime;

    // Ensure monotonic
    if (thread->vruntime < prev_vruntime) {
        thread->vruntime = prev_vruntime; // Handle theoretical wrap around
    }

    // Track min_vruntime to prevent unbounded lag
    if (thread->vruntime < rq->min_vruntime) {
         // Should not happen, but invariant safety
         rq->min_vruntime = thread->vruntime;
    } else if (thread->vruntime > rq->min_vruntime && rq->runnable_count == 1) {
         // Single active task drags the baseline forward
         rq->min_vruntime = thread->vruntime;
    }
}

