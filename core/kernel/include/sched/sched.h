#ifndef BHARAT_SCHED_H
#define BHARAT_SCHED_H

#include <stdint.h>
#include "mm.h"
#include "sched/ai_sched.h"
#include "list.h"
#include <lib/rbtree/rbtree.h>
#include "kernel_safety.h"
#include "spinlock.h"
#include "personality_ops.h"
#include <stdbool.h>
#include <bharat/constraints.h>
#include "bh_process_personality.h"
#include "bharat/kernel/ds/bh_mpsc_queue.h"

#define BH_TID_SLOT_BITS       16U
#define BH_TID_CORE_BITS       16U
#define BH_TID_GENERATION_BITS 32U

static inline uint16_t bh_tid_slot(uint64_t tid)
{
    return (uint16_t)(tid & 0xFFFFU);
}

static inline uint16_t bh_tid_home_core(uint64_t tid)
{
    return (uint16_t)((tid >> 16) & 0xFFFFU);
}

static inline uint16_t bh_tid_identity_home_cpu(uint64_t tid)
{
    return bh_tid_home_core(tid);
}

static inline uint32_t bh_tid_generation(uint64_t tid)
{
    return (uint32_t)(tid >> 32);
}

typedef struct {
    volatile uint32_t runnable_count;
    volatile uint32_t load_seq;
} sched_load_snapshot_t;

#define SCHED_REMOTE_CMD_CAPACITY 256U

typedef enum {
    SCHED_REMOTE_WAKE,
    SCHED_REMOTE_MIGRATE,
    SCHED_REMOTE_BLOCK,
    SCHED_REMOTE_YIELD,
    SCHED_REMOTE_ENQUEUE,
    SCHED_REMOTE_DEQUEUE,
    SCHED_REMOTE_HANDOFF,
    SCHED_REMOTE_SET_AFFINITY,
    SCHED_REMOTE_QUARANTINE,
    SCHED_REMOTE_STEAL_REQ,
    SCHED_REMOTE_MIGRATE_PREPARE,
    SCHED_REMOTE_MIGRATE_COMMIT,
    SCHED_REMOTE_MIGRATE_ROLLBACK,
    SCHED_REMOTE_SET_PRIORITY,
    SCHED_REMOTE_SET_THROTTLE,
    SCHED_REMOTE_TERMINATE,
    SCHED_REMOTE_REAP,
    SCHED_REMOTE_SET_CONSTRAINTS,
    SCHED_REMOTE_MIGRATE_RESERVE,
    SCHED_REMOTE_MIGRATE_COMMIT_IDENTITY,
    SCHED_REMOTE_MIGRATE_STAGE,
    SCHED_REMOTE_MIGRATE_ACTIVATE,
    SCHED_REMOTE_MIGRATE_RETIRE,
    SCHED_REMOTE_QUERY_STATE
} sched_remote_cmd_type_t;

typedef struct arch_ext_state arch_ext_state_t;

typedef struct {
    uint64_t regs[16];     // Offset 0
    uint64_t pc;           // Offset 128
    uint64_t sp;           // Offset 136
    uint64_t fpu_regs[32]; // Offset 144 (for inline FPU regs e.g. arm64 d8-d15, riscv fs0-fs11)
    arch_ext_state_t *ext; // Offset 400
} cpu_context_t;

typedef struct {
    uint64_t deadline_ms;
    uint64_t period_ms;
    uint64_t wcet_ms;
} bh_thread_attr_t;

typedef struct {
    uint16_t slot;
    uint16_t origin_cpu;
    uint32_t generation;
} sched_cmd_handle_t;

typedef struct {
    sched_cmd_handle_t handle;
    sched_remote_cmd_type_t type;
    uint32_t source_cpu;
    uint32_t target_cpu;
    uint64_t thread_id;
    uint64_t expected_thread_generation;
    uint32_t migration_epoch;
    uint32_t flags;
    uint32_t priority;
    bh_exec_constraints_k_t constraints;

    cpu_context_t context;
    int64_t vruntime;
    uint64_t absolute_deadline;
    bh_thread_attr_t rt_attr;
    uint16_t target_entity_slot;
    uint32_t target_entity_generation;
} sched_remote_cmd_envelope_t;

typedef struct {
    volatile uint64_t seq;
    sched_remote_cmd_envelope_t value;
} sched_cmd_slot_t;

typedef struct {
    sched_cmd_slot_t *slots;
    uint32_t capacity;
    uint32_t mask;
    volatile uint64_t head;
    uint64_t tail;
} sched_cmd_ring_t;

typedef enum {
    SCHED_COMPLETION_ACK = 0,
    SCHED_COMPLETION_NACK,
} sched_completion_kind_t;

typedef struct {
    sched_cmd_handle_t handle;   /* origin_cpu + slot + generation */
    int32_t result;
    uint16_t responder_cpu;
    uint8_t kind;
    uint8_t reserved;
} sched_remote_completion_t;

typedef struct {
    volatile uint64_t seq;
    sched_remote_completion_t value;
} sched_completion_slot_t;

typedef struct {
    sched_completion_slot_t *slots;
    uint32_t capacity;
    uint32_t mask;
    volatile uint64_t head;
    uint64_t tail;
} sched_completion_ring_t;

typedef struct {
    sched_cmd_ring_t cmd_ring;
    sched_cmd_slot_t cmd_slots[SCHED_REMOTE_CMD_CAPACITY];

    sched_completion_ring_t completion_ring;
    sched_completion_slot_t completion_slots[SCHED_REMOTE_CMD_CAPACITY];

    volatile uint32_t resched_pending;

    uint64_t submitted;
    uint64_t consumed;
    uint64_t full;
    uint64_t ipi_sent;
    uint64_t ipi_coalesced;
} sched_remote_inbox_t;

/*
 * Bharat-OS Process & Thread Management
 * Handles contexts from RTOS edge threading to datacenter high-throughput workloads.
 */

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
    SCHED_POLICY_ROUND_ROBIN = 0,
    SCHED_POLICY_CLOUD_FAIR  = 1,
    SCHED_POLICY_PRIORITY = 2,
    SCHED_POLICY_EDF = 3,
    SCHED_POLICY_RMS = 4
} sched_policy_t;

#include "trap/syscall_regs.h"
typedef bh_personality_id_t personality_type_t;


#define SCHED_MAX_PRIORITY 31U
#define MAX_PRIORITY_LEVELS (SCHED_MAX_PRIORITY + 1U)

#define BH_THREAD_FLAG_IDLE (1U << 0)

#define SCHED_CMD_BITMAP_WORDS 8

typedef enum {
    SCHED_REMOTE_CMD_EMPTY = 0,
    SCHED_REMOTE_CMD_RESERVED,
    SCHED_REMOTE_CMD_PENDING,
    SCHED_REMOTE_CMD_ACKED,
    SCHED_REMOTE_CMD_FAILED,
    SCHED_REMOTE_CMD_TIMEOUT
} sched_remote_cmd_state_t;

typedef enum {
    SCHED_MIG_NONE = 0,
    SCHED_MIG_RESERVE_SENT,
    SCHED_MIG_TARGET_RESERVED,
    SCHED_MIG_SOURCE_FROZEN,
    SCHED_MIG_TARGET_PREPARED,
    SCHED_MIG_OWNER_COMMIT_SENT,
    SCHED_MIG_OWNER_COMMITTED,
    SCHED_MIG_ACTIVATE_SENT,
    SCHED_MIG_TARGET_ACTIVE,
    SCHED_MIG_SOURCE_RETIRED,
    SCHED_MIG_ROLLBACK_SENT,
    SCHED_MIG_ROLLED_BACK,
    SCHED_MIG_RECONCILING,
    SCHED_MIG_FAILED
} sched_migration_state_t;

#define SCHED_MIGRATION_NONE          SCHED_MIG_NONE
#define SCHED_MIGRATION_PREPARE_SENT  SCHED_MIG_RESERVE_SENT
#define SCHED_MIGRATION_DEQUEUED      SCHED_MIG_SOURCE_FROZEN
#define SCHED_MIGRATION_COMMIT_SENT   SCHED_MIG_OWNER_COMMIT_SENT
#define SCHED_MIGRATION_COMMITTED     SCHED_MIG_TARGET_ACTIVE
#define SCHED_MIGRATION_ROLLBACK_SENT SCHED_MIG_ROLLBACK_SENT
#define SCHED_MIGRATION_FAILED        SCHED_MIG_FAILED

#define SCHED_RECENT_TXN_COUNT 256

typedef struct {
    sched_cmd_handle_t handle;

    uint64_t tid;
    uint32_t migration_epoch;
    uint16_t command_type;

    uint16_t outcome; // SCHED_COMPLETION_ACK or SCHED_COMPLETION_NACK
    int32_t result;

    uint64_t completed_tick;
} sched_recent_txn_t;

typedef struct {
    uint16_t cpu;
    uint16_t slot;
    uint32_t entity_generation;
    uint32_t migration_epoch;
} sched_owner_locator_t;

struct bh_thread;
typedef struct bh_thread bh_thread_t;

typedef struct {
    uint64_t tid;
    uint32_t entity_generation;
    uint32_t owner_cpu;
    uint32_t state; // thread_state_t values
    uint32_t priority;
    uint32_t base_priority;
    uint32_t affinity_mask;
    bh_exec_constraints_k_t constraints;
    int64_t vruntime;
    uint64_t absolute_deadline;
    bh_thread_attr_t rt_attr;
    cpu_context_t context;
    list_head_t run_node;
    list_head_t wait_node;
    struct rb_node cfs_node;
    struct rb_node edf_node;
    uint32_t migration_epoch;
    sched_migration_state_t migration_state;
    bool runnable;
    bool is_sleeping;
    bool is_blocked;
    uint8_t is_on_runqueue;
} sched_entity_t;

typedef struct {
    sched_entity_t entity;
    uint32_t generation;
    uint32_t next_free;
    uint8_t in_use;
} sched_entity_slot_t;

#define SCHED_MAX_LOCAL_ENTITIES 256U

#define COMPLETION_EMPTY     0U
#define COMPLETION_ARMED     1U
#define COMPLETION_PUBLISHED 2U

typedef struct {
    volatile uint32_t state; // COMPLETION_EMPTY, COMPLETION_ARMED, COMPLETION_PUBLISHED
    uint32_t generation;

    int32_t result;
    uint16_t responder_cpu;
    uint8_t kind; // SCHED_COMPLETION_ACK or SCHED_COMPLETION_NACK
    uint8_t reserved;

    uint32_t migration_epoch;

    union {
        sched_owner_locator_t locator;

        struct {
            uint32_t observed_state;
            sched_owner_locator_t owner;
            uint32_t owner_epoch;
        } query;
    } payload;
} sched_completion_cell_t;

typedef struct {
    uint64_t thread_id;
    uint32_t home_core_id;
    uint32_t bound_core_id;
    uint32_t owner_cpu;
    uint32_t state;
    uint32_t priority;
    uint32_t affinity_mask;
    uint32_t migration_state;
    uint32_t migration_epoch;
} sched_thread_snapshot_t;

typedef struct sched_remote_cmd {
    sched_cmd_handle_t handle;
    uint64_t cmd_id; // Keep cmd_id for any backward-compatible field lookup

    sched_remote_cmd_type_t type;
    volatile uint32_t state;

    uint32_t source_cpu;
    uint32_t target_cpu;

    uint64_t thread_id;
    uint64_t expected_thread_generation;

    uint32_t migration_epoch;
    int32_t result;

    uint64_t submit_tick;
    uint64_t deadline_tick;

    uint32_t flags;
    uint32_t priority;
    bh_exec_constraints_k_t constraints;

    cpu_context_t context;
    int64_t vruntime;
    uint64_t absolute_deadline;
    bh_thread_attr_t rt_attr;
    uint16_t target_entity_slot;
    uint32_t target_entity_generation;

    list_head_t list;
} sched_remote_cmd_t;

struct bh_process;
typedef struct bh_process bh_process_t;

struct thread_slot;
struct process_slot;

typedef enum {
    THREAD_FAULT_NONE = 0,
    THREAD_FAULT_SEGV,
    THREAD_FAULT_STACK_OVERFLOW,
    THREAD_FAULT_MIGRATION_ROLLBACK_FAILED,
} thread_fault_t;

typedef struct sched_rq {
    bh_thread_t* current_thread;
    bh_thread_t* idle_thread;

    // Priority / RT Scheduler
    list_head_t ready_queue[MAX_PRIORITY_LEVELS];
    uint32_t ready_bitmap;

    // CFS Scheduler
    struct rb_root cfs_runqueue;
    int64_t min_vruntime;

    // EDF Scheduler
    struct rb_root edf_runqueue;

    // RT Admissions
    uint64_t rt_budget_used;
    uint64_t rt_budget_total;

    // Remote Scheduler Command Inbox (MPSC Lock-Free)
    sched_remote_inbox_t remote;

    // Outbound Command Pool & load snapshot
    sched_remote_cmd_t outbound_cmds[SCHED_REMOTE_CMD_CAPACITY];
    volatile uint32_t outbound_alloc_bitmap[SCHED_CMD_BITMAP_WORDS];
    sched_load_snapshot_t load_snapshot;

    // Flag to avoid IPI storms
    uint8_t resched_pending;

    // Debug counters
    uint64_t remote_enqueues;
    uint64_t ipi_sent;
    uint64_t ipi_coalesced;
    uint64_t inbox_drains;
    uint64_t remote_preemptions;

    uint32_t runnable_count;
    list_head_t sleeping_list;
    list_head_t blocked_list;
    uint64_t total_ticks;
    uint64_t context_switches;
    uint32_t throttled;
    bool sched_isolated;
    uint32_t isolation_reason;
    spinlock_t lock;

    // Deferred reaping queue
    uint32_t reap_head;
    uint32_t reap_tail;

    // Per-core object pools
    uint32_t free_thread_head;
    uint32_t free_process_head;
    struct thread_slot *threads;
    struct process_slot *processes;
    struct thread_slot *bootstrap_threads;
    void *mutex_owners;
    void *pending_suggestions;
    uint8_t *bootstrap_stacks;

    // Per-core execution entity pool
    sched_entity_slot_t entities[SCHED_MAX_LOCAL_ENTITIES];
    uint32_t free_entity_head;

    // Outbound command completion cells
    sched_completion_cell_t completions[SCHED_REMOTE_CMD_CAPACITY];

    // Recent transaction cache
    sched_recent_txn_t recent_txns[SCHED_RECENT_TXN_COUNT];
    uint32_t recent_txn_head;
} sched_rq_t;

typedef sched_rq_t sched_core_state_t;

typedef struct {
    bh_thread_t* head;
    bh_thread_t* tail;
} wait_queue_t;

struct bh_thread {
    uint64_t thread_id;
    uint64_t process_id;
    bh_process_t* process;

    bh_exec_constraints_k_t constraints;

    // Ownership and lookup metadata
    uint32_t home_core_id;
    uint32_t generation;

    // CPU Architectural Context (Registers)
    void* cpu_context;

    // Kernel Stack
    virt_addr_t kernel_stack;

    thread_state_t state;
    uint32_t priority;

    // Priority Inheritance (Hard-RT / OpenRAN profile)
    uint32_t base_priority;
    void* waiting_on_lock; // Mutex the thread is waiting for

    // Personality tagging for subsystems (e.g., Linux, Android, Windows)
    personality_type_t personality;

    // Capability and accounting metadata
    void* capability_list;
    mm_color_config_t mm_color_policy;
    uint64_t time_slice_ms;
    uint64_t cpu_time_consumed;

    // CFS Scheduler metadata
    int64_t vruntime;
    uint32_t weight;

    // EDF Scheduler metadata
    uint64_t absolute_deadline_ms;

    uint8_t preferred_numa_node;
    ai_sched_context_t* ai_sched_ctx;
    uint64_t context_switch_count;
    bh_thread_attr_t rt_attr;
    uint64_t wake_deadline_ms;
    uint32_t bound_core_id;
    uint32_t affinity_mask;

    // Scheduling context for distributed execution groups (DEGs)
    struct sched_context* sched_ctx;

    // Next thread in a wait queue
    bh_thread_t* next_waiter;

    // IPC blocking state
    uint64_t ipc_deadline_ticks;
    int ipc_wakeup_reason;

    // Fault state
    thread_fault_t pending_fault;
    bool fault_pending;

    uint32_t flags;

    // Phase K0: Invariant tracking
    uint32_t owner_cpu;
    uint64_t sched_generation;
    thread_sched_owner_state_t owner_state;
    bool enqueued;

    sched_owner_locator_t owner_locator;

    // Migration state
    uint32_t migration_target_cpu;
    sched_migration_state_t migration_state;
    uint32_t migration_epoch;
};

int thread_raise_fault(bh_thread_t *thread, thread_fault_t fault);
int sched_mark_thread_terminated(bh_thread_t *thread);
int sched_quarantine_thread(bh_thread_t *thread, uint32_t reason);

struct bh_process {
    uint64_t process_id;
    address_space_t* addr_space;
    bh_thread_t* main_thread;

    // Ownership and lookup metadata
    uint32_t home_core_id;
    uint32_t generation;

    // Personality tagging for subsystems (e.g., Linux, Android, Windows)
    bh_process_personality_t personality;

    // Ops mapping syscalls/faults to personality specific behavior
    const struct personality_ops* personality_ops;

    // Capability-based security context would be linked here
    void* security_sandbox_ctx;

    // Ownership tracking
    uint32_t owner_core_id;
    uint64_t object_id;
};

// Scheduler Core
void sched_init(void);

// Create process and main thread
bh_process_t* process_create(const char* name);
int process_destroy(bh_process_t* process);
bh_thread_t* thread_create(bh_process_t* parent, void (*entry_point)(void));
bh_thread_t* thread_create_detached(bh_process_t* parent, void (*entry_point)(void));
int thread_destroy(bh_thread_t* thread);

// Current Context Helpers
bh_process_t* sched_current_process(void);
address_space_t* sched_current_aspace(void);
struct capability_table* sched_current_cap_table(void);

// Wait Queues
void sched_wait_queue_init(wait_queue_t* queue);
void sched_wait_queue_enqueue(wait_queue_t* queue, bh_thread_t* thread);
bh_thread_t* sched_wait_queue_dequeue(wait_queue_t* queue);

// Wait Queue State
void sched_block(void);

// L0 layer access (for testing)
bh_thread_t *sched_pick_next_ready_l0(uint32_t core_id);

// Context Switching
void bh_thread_yield(void);
void sched_on_timer_tick(void);
bh_thread_t* sched_current_thread(void);
uint64_t sched_get_ticks(void);
void sched_set_policy(sched_policy_t policy);
void sched_reschedule(void);
bh_thread_t* sched_current(void);
int sched_enqueue(bh_thread_t* thread, uint32_t core_id);
void sched_sleep(uint64_t millis);
int sched_wake_tid(uint64_t tid);
int sched_wake_tid_with_priority(uint64_t tid, uint32_t priority);

void sched_wakeup(bh_thread_t *thread);
void sched_wakeup_with_priority(bh_thread_t *thread, uint32_t wakeup_priority);

// AI governor integration helpers
int sched_set_thread_priority(uint64_t tid, uint32_t new_priority);
int sched_set_thread_preferred_node(uint64_t tid, uint8_t node_id);
int sched_ai_apply_suggestion(const ai_suggestion_t* suggestion);
int sched_enqueue_ai_suggestion(const ai_suggestion_t* suggestion);
int sched_migrate_task(bh_thread_t *thread, uint32_t new_node);
int sched_migrate_tid(uint64_t tid, uint32_t target_cpu);
int sched_set_priority(uint64_t tid, uint32_t priority);
int sched_set_affinity(uint64_t tid, uint32_t mask);
int sched_terminate_tid(uint64_t tid);
int sched_quarantine_tid(uint64_t tid, uint32_t reason);
int sched_throttle_core(uint32_t core_id);

// Cross-core remote handoff
int sched_request_handoff_tid(uint64_t tid, uint32_t target_cpu, uint32_t auth_token);
kstatus_t sched_get_thread_snapshot(uint64_t tid, sched_thread_snapshot_t *out);

// RT Scheduler Admissions
int sched_admission_edf(bh_thread_t* thread, uint64_t wcet_ms, uint64_t period_ms, uint64_t deadline_ms);
int sched_admission_rms(bh_thread_t* thread, uint64_t wcet_ms, uint64_t period_ms);

int sched_set_constraints(uint64_t tid, const bh_exec_constraints_k_t *c);
int sched_get_constraints(uint64_t tid, bh_exec_constraints_k_t *c);
bool sched_thread_exists(uint64_t tid);

// System-call style entry points used by trap/syscall layer
int sched_sys_thread_create(bh_process_t* parent, void (*entry_point)(void), uint64_t* out_tid);
int sched_sys_thread_destroy(uint64_t tid);
int sched_sys_sleep(uint64_t millis);
int sched_sys_set_priority(uint64_t tid, uint32_t new_priority);
int sched_sys_set_affinity(uint64_t tid, uint32_t affinity_mask);
int sched_sys_intent_set(uint64_t tid, const void* intent);
int sched_sys_intent_get(uint64_t tid, void* intent);

// Priority Inheritance support
void sched_inherit_priority(bh_thread_t* thread, uint32_t new_priority);
void sched_restore_priority(bh_thread_t* thread);
void sched_on_mutex_wait(bh_thread_t* waiter, void* mutex);
void sched_on_mutex_acquire(bh_thread_t* owner, void* mutex);
void sched_on_mutex_release(bh_thread_t* owner, void* mutex);

// Multikernel IPC integration stub
void sched_notify_ipc_ready(uint32_t core_id, uint32_t msg_type);

#ifdef Profile_RTOS
// Tickless operation for Hard-RT OpenRAN
void sched_disable_tick_for_core(uint32_t core_id);
#endif

sched_rq_t *sched_local_rq(void);
void sched_assert_local_rq(sched_rq_t *rq);
kstatus_t sched_remote_submit(uint32_t target_cpu, const sched_remote_cmd_t *cmd);
void sched_remote_cmd_release(sched_remote_cmd_t *cmd);

kstatus_t sched_cmd_ring_init(sched_cmd_ring_t *q, sched_cmd_slot_t *slots, uint32_t capacity);
kstatus_t sched_cmd_ring_push(sched_cmd_ring_t *q, const sched_remote_cmd_envelope_t *value);
kstatus_t sched_cmd_ring_pop(sched_cmd_ring_t *q, sched_remote_cmd_envelope_t *out_value);
bool sched_cmd_ring_empty(const sched_cmd_ring_t *q);

kstatus_t sched_completion_ring_init(sched_completion_ring_t *q, sched_completion_slot_t *slots, uint32_t capacity);
kstatus_t sched_completion_ring_push(sched_completion_ring_t *q, const sched_remote_completion_t *value);
kstatus_t sched_completion_ring_pop(sched_completion_ring_t *q, sched_remote_completion_t *out_value);
bool sched_completion_ring_empty(const sched_completion_ring_t *q);
kstatus_t sched_read_load_snapshot(uint32_t cpu, sched_load_snapshot_t *out);
bool sched_read_isolated_snapshot(uint32_t cpu);
kstatus_t sched_migration_transition(bh_thread_t *thread, sched_migration_state_t expected, sched_migration_state_t next);
void sched_remote_cmd_poll_timeouts(void);

#endif // BHARAT_SCHED_H
