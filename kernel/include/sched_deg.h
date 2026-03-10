#ifndef BHARAT_SCHED_DEG_H
#define BHARAT_SCHED_DEG_H

#include <stdint.h>
#include "sched.h"

typedef enum {
    DEG_STRICT
} deg_mode_t;

typedef enum {
    DEG_ACTIVE,
    DEG_WAITING_RELEASE,
    DEG_DEGRADED,
    DEG_ABORTED
} deg_state_t;

#define MAX_DEG_MEMBERS 16

typedef struct dist_exec_group {
    uint32_t deg_id;
    deg_mode_t mode;
    deg_state_t state;
    uint32_t member_count;
    struct kthread* members[MAX_DEG_MEMBERS];
    uint32_t ready_bitmap;
    uint32_t running_bitmap;
    uint64_t release_epoch;
    uint64_t release_window_ns;
} dist_exec_group_t;

#define SCHED_CTX_FLAG_STRICT_COSCHED 1

typedef struct sched_context {
    dist_exec_group_t* deg;
    uint32_t flags;
    uint32_t local_sched_class;
    uint32_t local_priority;
    uint32_t local_affinity;
} sched_context_t;

dist_exec_group_t* deg_create(void);
int deg_add_member(dist_exec_group_t* deg, struct kthread* thread, uint32_t core, uint32_t local_sched_class);
int deg_activate(dist_exec_group_t* deg);
int deg_mark_ready(struct kthread* thread);
int deg_release_if_ready(dist_exec_group_t* deg);
void deg_block_member(struct kthread* thread, uint32_t reason);
void deg_abort_epoch(dist_exec_group_t* deg);

#endif // BHARAT_SCHED_DEG_H
