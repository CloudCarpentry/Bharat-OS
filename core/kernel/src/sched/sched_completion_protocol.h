#ifndef BHARAT_SCHED_COMPLETION_PROTOCOL_H
#define BHARAT_SCHED_COMPLETION_PROTOCOL_H

#include "sched/sched.h"

void sched_protocol_drain_completions(sched_rq_t *rq);
void sched_protocol_process_deadlines(sched_rq_t *rq, bh_ktime_t current_time);

#endif
