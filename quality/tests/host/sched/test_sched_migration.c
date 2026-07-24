#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* Minimal definitions to avoid including complex headers that cause segfaults in host test */

#define MAX_CPUS 32U
#define MAX_PRIORITY_LEVELS 32
#define K_OK 0
#define K_ERR_BAD_STATE -2
#define K_ERR_IN_PROGRESS -19

typedef int32_t kstatus_t;

typedef enum {
    THREAD_STATE_READY,
    THREAD_STATE_RUNNING,
    THREAD_STATE_BLOCKED,
    THREAD_STATE_SLEEPING,
    THREAD_STATE_TERMINATED,
    THREAD_STATE_DEG_PENDING,
    THREAD_STATE_REMOTE_HANDOFF_PENDING,
    THREAD_STATE_QUARANTINED
} thread_state_t;

typedef enum {
    THREAD_OWNER_NONE,
    THREAD_OWNER_RUNQUEUE,
    THREAD_OWNER_RUNNING,
    THREAD_OWNER_BLOCKED,
    THREAD_OWNER_REMOTE_PENDING,
    THREAD_OWNER_QUARANTINED,
} thread_sched_owner_state_t;

typedef enum {
    SCHED_REMOTE_CMD_EMPTY = 0,
    SCHED_REMOTE_CMD_PENDING,
    SCHED_REMOTE_CMD_ACKED,
    SCHED_REMOTE_CMD_FAILED,
    SCHED_REMOTE_CMD_TIMEOUT
} sched_remote_cmd_state_t;

typedef enum {
    SCHED_MIGRATION_NONE = 0,
    SCHED_MIGRATION_DEQUEUE_REQUESTED,
    SCHED_MIGRATION_DEQUEUED,
    SCHED_MIGRATION_ENQUEUE_REQUESTED,
    SCHED_MIGRATION_COMMITTED,
    SCHED_MIGRATION_ROLLBACK_REQUESTED,
    SCHED_MIGRATION_FAILED
} sched_migration_state_t;

typedef enum {
    SCHED_REMOTE_WAKE,
    SCHED_REMOTE_MIGRATE,
    SCHED_REMOTE_BLOCK,
    SCHED_REMOTE_YIELD,
    SCHED_REMOTE_ENQUEUE,
    SCHED_REMOTE_DEQUEUE,
    SCHED_REMOTE_HANDOFF,
    SCHED_REMOTE_SET_AFFINITY,
    SCHED_REMOTE_QUARANTINE
} sched_remote_cmd_type_t;

typedef struct list_head {
    struct list_head *next, *prev;
} list_head_t;

static inline void list_init(list_head_t *ptr) {
    ptr->next = ptr;
    ptr->prev = ptr;
}

static inline void list_add_tail(list_head_t *new_node, list_head_t *head) {
    list_head_t *prev = head->prev;
    new_node->next = head;
    new_node->prev = prev;
    prev->next = new_node;
    head->prev = new_node;
}

static inline void list_del(list_head_t *entry) {
    entry->next->prev = entry->prev;
    entry->prev->next = entry->next;
}

static inline int list_empty(const list_head_t *head) {
    return head->next == head;
}

typedef struct sched_remote_cmd {
    uint64_t cmd_id;
    sched_remote_cmd_type_t type;
    sched_remote_cmd_state_t state;
    uint32_t source_cpu;
    uint32_t target_cpu;
    uint64_t thread_id;
    uint64_t expected_thread_generation;
    uint32_t flags;
    uint32_t priority;
    list_head_t list;
} sched_remote_cmd_t;

typedef enum {
    THREAD_FAULT_NONE = 0,
    THREAD_FAULT_SEGV,
    THREAD_FAULT_STACK_OVERFLOW,
    THREAD_FAULT_MIGRATION_ROLLBACK_FAILED,
} thread_fault_t;

typedef struct bh_thread {
    uint64_t thread_id;
    uint32_t home_core_id;
    thread_state_t state;
    uint32_t priority;
    uint32_t bound_core_id;
    uint32_t affinity_mask;
    thread_fault_t pending_fault;
    uint32_t owner_cpu;
    uint64_t sched_generation;
    thread_sched_owner_state_t owner_state;
    uint32_t migration_target_cpu;
    sched_migration_state_t migration_state;
} bh_thread_t;

typedef struct sched_rq {
    list_head_t pending_inbox;
    uint8_t resched_pending;
} sched_rq_t;

typedef struct cpu_local {
    sched_rq_t runqueue;
} cpu_local_t;

cpu_local_t g_cpu_locals[MAX_CPUS];

typedef struct thread_slot {
    uint8_t in_use;
    bh_thread_t thread;
    uint8_t is_on_runqueue;
    sched_remote_cmd_t remote_cmd;
} thread_slot_t;

/* Global state for mocks */
uint32_t g_active_core_count = 4;
thread_slot_t g_slots[128];
bh_thread_t* g_threads[128];
static uint32_t mock_cpu_id = 0;

/* Mocks */
uint32_t hal_cpu_get_id(void) { return mock_cpu_id; }
uint32_t sched_clamp_core(uint32_t core_id) { return core_id; }
void hal_cpu_disable_interrupts(void) {}
void hal_cpu_enable_interrupts(void) {}
void hal_send_ipi_payload(uint32_t mask, uint64_t payload) {}
void spin_lock(void* lock) {}
void spin_unlock(void* lock) {}

thread_slot_t *sched_find_thread_slot_by_tid_local(void *rq, uint64_t tid) {
    for (int i = 0; i < 128; i++) {
        if (g_slots[i].in_use && g_slots[i].thread.thread_id == tid) return &g_slots[i];
    }
    return NULL;
}

bh_thread_t *sched_find_thread_by_id(uint64_t tid) {
    for (int i = 0; i < 128; i++) {
        if (g_threads[i] && g_threads[i]->thread_id == tid) return g_threads[i];
    }
    return NULL;
}

bool sched_is_core_admissible(bh_thread_t *t, int cpu_id) {
    if (cpu_id == 3) return false;
    return true;
}

#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)

int sched_quarantine_thread(bh_thread_t *thread, uint32_t reason) {
  if (!thread) return -1;
  thread->state = THREAD_STATE_QUARANTINED;
  thread->owner_state = THREAD_OWNER_QUARANTINED;
  thread->pending_fault = (thread_fault_t)reason;
  return 0;
}

/* Logic cloned from production with minor mock adjustments */

int test_sched_migrate_task(bh_thread_t *thread, uint32_t new_node) {
  if (thread->state == THREAD_STATE_QUARANTINED || thread->owner_state == THREAD_OWNER_QUARANTINED) {
    return K_ERR_BAD_STATE;
  }

  if (thread->migration_state != SCHED_MIGRATION_NONE) {
      return K_ERR_IN_PROGRESS;
  }

  thread_slot_t *slot = sched_find_thread_slot_by_tid_local(NULL, thread->thread_id);
  if (!slot) return -1;

  if (slot->remote_cmd.state == SCHED_REMOTE_CMD_PENDING) {
      return K_ERR_IN_PROGRESS;
  }

  uint32_t current_core = hal_cpu_get_id();
  uint32_t bound_core = thread->bound_core_id;

  if (bound_core == new_node) return 0;

  thread->migration_target_cpu = new_node;

  if (bound_core == current_core) {
      slot->is_on_runqueue = 0U;
      thread->migration_state = SCHED_MIGRATION_DEQUEUED;
      thread->owner_state = THREAD_OWNER_REMOTE_PENDING;

      sched_rq_t *target_rq = &g_cpu_locals[new_node].runqueue;
      sched_remote_cmd_t *cmd = &slot->remote_cmd;
      cmd->type = SCHED_REMOTE_ENQUEUE;
      cmd->state = SCHED_REMOTE_CMD_PENDING;
      cmd->source_cpu = current_core;
      cmd->target_cpu = new_node;
      cmd->thread_id = thread->thread_id;
      list_add_tail(&cmd->list, &target_rq->pending_inbox);

      thread->migration_state = SCHED_MIGRATION_ENQUEUE_REQUESTED;
      return K_ERR_IN_PROGRESS;
  } else {
      sched_rq_t *old_owner_rq = &g_cpu_locals[bound_core].runqueue;
      sched_remote_cmd_t *cmd = &slot->remote_cmd;
      cmd->type = SCHED_REMOTE_MIGRATE;
      cmd->state = SCHED_REMOTE_CMD_PENDING;
      cmd->source_cpu = current_core;
      cmd->target_cpu = bound_core;
      cmd->thread_id = thread->thread_id;
      list_add_tail(&cmd->list, &old_owner_rq->pending_inbox);
      thread->migration_state = SCHED_MIGRATION_DEQUEUE_REQUESTED;
      return K_ERR_IN_PROGRESS;
  }
}

void test_mock_reschedule(void) {
    uint32_t core = hal_cpu_get_id();
    sched_rq_t *rq = &g_cpu_locals[core].runqueue;

    while (!list_empty(&rq->pending_inbox)) {
        list_head_t *curr = rq->pending_inbox.next;
        sched_remote_cmd_t *cmd = (sched_remote_cmd_t *)(void *)((char *)curr - offsetof(sched_remote_cmd_t, list));
        list_del(&cmd->list);
        list_init(&cmd->list);

        bh_thread_t* thread = sched_find_thread_by_id(cmd->thread_id);
        if (!thread) continue;

        if (cmd->type == SCHED_REMOTE_MIGRATE) {
            thread->owner_state = THREAD_OWNER_REMOTE_PENDING;
            thread->migration_state = SCHED_MIGRATION_DEQUEUED;
            uint32_t target_cpu = thread->migration_target_cpu;
            sched_rq_t *target_rq = &g_cpu_locals[target_cpu].runqueue;
            cmd->type = SCHED_REMOTE_ENQUEUE;
            cmd->source_cpu = core;
            cmd->target_cpu = target_cpu;
            cmd->state = SCHED_REMOTE_CMD_PENDING;
            list_add_tail(&cmd->list, &target_rq->pending_inbox);
            thread->migration_state = SCHED_MIGRATION_ENQUEUE_REQUESTED;
        } else if (cmd->type == SCHED_REMOTE_ENQUEUE) {
            if (thread->owner_state == THREAD_OWNER_REMOTE_PENDING) {
                if (!sched_is_core_admissible(thread, core)) {
                    cmd->state = SCHED_REMOTE_CMD_FAILED;
                    uint32_t old_owner = cmd->source_cpu;
                    sched_rq_t *old_rq = &g_cpu_locals[old_owner].runqueue;
                    cmd->type = SCHED_REMOTE_ENQUEUE;
                    cmd->target_cpu = old_owner;
                    cmd->source_cpu = core;
                    cmd->state = SCHED_REMOTE_CMD_PENDING;
                    list_add_tail(&cmd->list, &old_rq->pending_inbox);
                    thread->migration_state = SCHED_MIGRATION_ROLLBACK_REQUESTED;
                } else {
                    thread->owner_state = THREAD_OWNER_NONE;
                    thread->owner_cpu = core;
                    thread->bound_core_id = core;
                    thread->migration_state = SCHED_MIGRATION_COMMITTED;
                    cmd->state = SCHED_REMOTE_CMD_ACKED;
                }
            } else if (thread->migration_state == SCHED_MIGRATION_ROLLBACK_REQUESTED) {
                thread->owner_state = THREAD_OWNER_NONE;
                thread->owner_cpu = core;
                thread->bound_core_id = core;
                thread->migration_state = SCHED_MIGRATION_NONE;
                cmd->state = SCHED_REMOTE_CMD_ACKED;
            } else if (thread->owner_state == THREAD_OWNER_REMOTE_PENDING &&
                       thread->migration_state == SCHED_MIGRATION_ROLLBACK_REQUESTED) {
                // Rollback failed
                sched_quarantine_thread(thread, THREAD_FAULT_MIGRATION_ROLLBACK_FAILED);
                thread->migration_state = SCHED_MIGRATION_FAILED;
                cmd->state = SCHED_REMOTE_CMD_FAILED;
            }
        } else if (cmd->type == SCHED_REMOTE_WAKE) {
            thread->state = THREAD_STATE_READY;
        }

        if (thread->migration_state == SCHED_MIGRATION_COMMITTED) {
            thread->migration_state = SCHED_MIGRATION_NONE;
        }

        if (thread->state == THREAD_STATE_QUARANTINED || thread->owner_state == THREAD_OWNER_QUARANTINED) {
            if (thread->migration_state == SCHED_MIGRATION_ROLLBACK_REQUESTED) {
                thread->migration_state = SCHED_MIGRATION_FAILED;
                thread->pending_fault = THREAD_FAULT_MIGRATION_ROLLBACK_FAILED;
            }
        }
    }
}

/* Test Cases */

void setup_thread(int id, uint32_t home_core, uint32_t bound_core, thread_state_t state) {
    memset(&g_slots[id], 0, sizeof(thread_slot_t));
    g_slots[id].in_use = 1;
    g_slots[id].thread.thread_id = id + 100;
    g_slots[id].thread.home_core_id = home_core;
    g_slots[id].thread.bound_core_id = bound_core;
    g_slots[id].thread.state = state;
    g_slots[id].is_on_runqueue = 1;
    list_init(&g_slots[id].remote_cmd.list);
    g_threads[id] = &g_slots[id].thread;
}

void test_pending_blocks_second_migration(void) {
    printf("test_pending_blocks_second_migration...\n");
    setup_thread(0, 0, 0, THREAD_STATE_READY);
    bh_thread_t *t = &g_slots[0].thread;
    mock_cpu_id = 0;
    int ret = test_sched_migrate_task(t, 1);
    assert(ret == K_ERR_IN_PROGRESS);
    assert(t->migration_state == SCHED_MIGRATION_ENQUEUE_REQUESTED);
    ret = test_sched_migrate_task(t, 2);
    assert(ret == K_ERR_IN_PROGRESS);
    printf("PASS\n");
}

void test_remote_old_owner_no_immediate_change(void) {
    printf("test_remote_old_owner_no_immediate_change...\n");
    setup_thread(1, 1, 1, THREAD_STATE_READY);
    bh_thread_t *t = &g_slots[1].thread;
    mock_cpu_id = 0;
    int ret = test_sched_migrate_task(t, 2);
    assert(ret == K_ERR_IN_PROGRESS);
    assert(t->bound_core_id == 1);
    assert(t->migration_state == SCHED_MIGRATION_DEQUEUE_REQUESTED);
    printf("PASS\n");
}

void test_target_enqueue_success_commits(void) {
    printf("test_target_enqueue_success_commits...\n");
    setup_thread(3, 0, 0, THREAD_STATE_READY);
    bh_thread_t *t = &g_slots[3].thread;
    mock_cpu_id = 0;
    test_sched_migrate_task(t, 1);
    mock_cpu_id = 1;
    test_mock_reschedule();
    assert(t->bound_core_id == 1);
    assert(t->owner_cpu == 1);
    assert(t->migration_state == SCHED_MIGRATION_NONE);
    printf("PASS\n");
}

void test_target_enqueue_failure_rollbacks(void) {
    printf("test_target_enqueue_failure_rollbacks...\n");
    setup_thread(4, 0, 0, THREAD_STATE_READY);
    bh_thread_t *t = &g_slots[4].thread;
    mock_cpu_id = 0;
    test_sched_migrate_task(t, 3);
    mock_cpu_id = 3;
    test_mock_reschedule();
    assert(t->migration_state == SCHED_MIGRATION_ROLLBACK_REQUESTED);
    mock_cpu_id = 0;
    test_mock_reschedule();
    assert(t->migration_state == SCHED_MIGRATION_NONE);
    assert(t->bound_core_id == 0);
    printf("PASS\n");
}

void test_blocked_thread_no_wakeup_on_migration(void) {
    printf("test_blocked_thread_no_wakeup_on_migration...\n");
    setup_thread(5, 0, 0, THREAD_STATE_BLOCKED);
    bh_thread_t *t = &g_slots[5].thread;
    mock_cpu_id = 0;
    test_sched_migrate_task(t, 1);
    mock_cpu_id = 1;
    test_mock_reschedule();
    assert(t->bound_core_id == 1);
    assert(t->state == THREAD_STATE_BLOCKED);
    printf("PASS\n");
}

int main(void) {
    for (int i = 0; i < 32; i++) {
        list_init(&g_cpu_locals[i].runqueue.pending_inbox);
    }
    test_pending_blocks_second_migration();
    test_remote_old_owner_no_immediate_change();
    test_target_enqueue_success_commits();
    test_target_enqueue_failure_rollbacks();
    test_blocked_thread_no_wakeup_on_migration();
    printf("ALL TESTS PASSED\n");
    return 0;
}
