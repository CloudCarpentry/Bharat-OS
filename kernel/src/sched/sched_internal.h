#ifndef BHARAT_SCHED_INTERNAL_H
#define BHARAT_SCHED_INTERNAL_H

#include "sched/sched.h"
#include <bharat/cpu_local.h>
#include "list.h"
#include "bharat_config.h"

#define SCHED_MAX_THREADS 128U
#define SCHED_MAX_PROCESSES 32U

typedef struct {
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
} thread_slot_t;

typedef struct {
  uint8_t in_use;
  uint32_t next_free;
  kprocess_t process;
} process_slot_t;

extern thread_slot_t g_threads[SCHED_MAX_THREADS];
extern thread_slot_t g_bootstrap_threads[MAX_SUPPORTED_CORES][2];
extern process_slot_t g_processes[SCHED_MAX_PROCESSES];
extern uint8_t g_sched_initialized;

#if defined(BHARAT_ENABLE_KERNEL_SELFTESTS)
extern uint32_t g_sched_test_core_count;
#endif

void sched_reset_core_runqueues(void);

#endif // BHARAT_SCHED_INTERNAL_H
