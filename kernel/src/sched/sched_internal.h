#ifndef BHARAT_SCHED_INTERNAL_H
#define BHARAT_SCHED_INTERNAL_H

#include "sched/sched.h"
#include <bharat/cpu_local.h>
#include "list.h"
#include "bharat_config.h"
#include "sched/sched_deg.h"
#include "core/multikernel.h"
#include "hal/hal.h"
#include "slab.h"
#include "ipc_async.h"
#include "lib/base/string.h"
#include "arch/arch_ext_state.h"

#define SCHED_MAX_THREADS 128U
#define SCHED_MAX_PROCESSES 32U
#define SCHED_MAX_PENDING_SUGGESTIONS 64U
#define SCHED_AFFINITY_ANY 0xFFFFFFFFU
#define CFS_NICE_0_WEIGHT 1024U

typedef struct {
  ai_suggestion_t queue[SCHED_MAX_PENDING_SUGGESTIONS];
  uint32_t head;
  uint32_t tail;
} suggestion_queue_t;

typedef struct {
  void *mutex;
  kthread_t *owner;
} mutex_owner_entry_t;

typedef struct thread_slot {
  uint8_t in_use;
  uint8_t is_bootstrap;
  uint32_t next_free;
  kthread_t thread;
  cpu_context_t context;
  ai_sched_context_t ai_ctx;
  list_head_t run_node;
  list_head_t wait_node; // Used for both sleeping_list and blocked_list
  uint32_t reap_next;
  uint8_t reap_pending;
  uint8_t is_on_runqueue;
  uint8_t is_sleeping;
  uint8_t is_blocked;
  uint32_t creation_core_id;
} thread_slot_t;

typedef struct process_slot {
  uint8_t in_use;
  uint32_t next_free;
  kprocess_t process;
} process_slot_t;

extern uint8_t g_sched_initialized;
extern sched_policy_t g_policy;
extern uint32_t g_active_core_count;

#if defined(BHARAT_ENABLE_KERNEL_SELFTESTS)
extern uint32_t g_sched_test_core_count;
#endif

void sched_reset_core_runqueues(void);
thread_slot_t *sched_find_thread_slot_by_tid_local(sched_rq_t *rq, uint64_t tid);
thread_slot_t *sched_resolve_tid_owner_slow(uint64_t tid);
uint32_t sched_clamp_core(uint32_t core_id);
int sched_mark_thread_terminated(kthread_t *thread);
void sched_reap_terminated_threads(void);
void sched_process_pending_ai_suggestions(void);
void sched_sleep_enqueue(thread_slot_t *slot, uint32_t core_id);
void sched_sleep_dequeue(thread_slot_t *slot);
void sched_block_enqueue(thread_slot_t *slot, uint32_t core_id);
void sched_block_dequeue(thread_slot_t *slot);
void sched_ready_bitmap_set(sched_rq_t *rq, uint32_t prio);
void sched_ready_bitmap_clear_if_empty(sched_rq_t *rq, uint32_t prio);
int sched_pick_priority_from_bitmap(const sched_rq_t *rq, int highest);
uint32_t sched_run_queue_depth(uint32_t core_id);
kthread_t *sched_pick_next_ready(uint32_t core_id);
void sched_cfs_enqueue(sched_rq_t *rq, kthread_t *thread);
void sched_cfs_dequeue(sched_rq_t *rq, kthread_t *thread);
kthread_t *sched_cfs_pick_next(sched_rq_t *rq);
void sched_cfs_update_vruntime(sched_rq_t *rq, kthread_t *thread, uint64_t delta_exec);
void sched_edf_enqueue(sched_rq_t *rq, kthread_t *thread);
void sched_edf_dequeue(sched_rq_t *rq, kthread_t *thread);
kthread_t *sched_edf_pick_next(sched_rq_t *rq);
kthread_t *sched_find_mutex_owner(void *mutex);
void sched_register_mutex_owner(void *mutex, kthread_t *owner);
void sched_unregister_mutex_owner(void *mutex, kthread_t *owner);
kthread_t *sched_find_thread_by_id(uint64_t tid);
void sched_balance_once(void);
void sched_detach_thread_from_queues(thread_slot_t *slot);
bool sched_is_core_admissible(kthread_t *t, int cpu_id);
void sched_switch_to(kthread_t *next, uint32_t core_id);
void sched_update_telemetry(kthread_t *thread);
void sched_validate_rq(sched_rq_t *rq);

#endif // BHARAT_SCHED_INTERNAL_H
