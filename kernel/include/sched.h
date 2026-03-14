#ifndef BHARAT_SCHED_H
#define BHARAT_SCHED_H

#include <stdint.h>
#include "mm.h"
#include "advanced/ai_sched.h"

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
    THREAD_STATE_DEG_PENDING
} thread_state_t;

typedef enum {
    SCHED_POLICY_ROUND_ROBIN = 0,
    SCHED_POLICY_CLOUD_FAIR  = 1,
    SCHED_POLICY_PRIORITY = 2,
    SCHED_POLICY_EDF = 3,
    SCHED_POLICY_RMS = 4
} sched_policy_t;

typedef enum {
    PERSONALITY_NATIVE = 0,
    PERSONALITY_LINUX = 1
} personality_type_t;


#define SCHED_MAX_PRIORITY 31U

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
} kthread_attr_t;

typedef struct kthread kthread_t;

typedef struct {
    uint32_t core_id;
    kthread_t* current;
    kthread_t* idle;
    uint64_t total_ticks;
    uint64_t context_switches;
    uint32_t throttled;
} sched_core_t;

typedef struct {
    kthread_t* head;
    kthread_t* tail;
} wait_queue_t;

struct kthread {
    uint64_t thread_id;
    uint64_t process_id;

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
    uint8_t preferred_numa_node;
    ai_sched_context_t* ai_sched_ctx;
    uint64_t context_switch_count;
    kthread_attr_t rt_attr;
    uint64_t wake_deadline_ms;
    uint32_t bound_core_id;
    uint32_t affinity_mask;

    // Scheduling context for distributed execution groups (DEGs)
    struct sched_context* sched_ctx;

    // Next thread in a wait queue
    kthread_t* next_waiter;

    // IPC blocking state
    uint64_t ipc_deadline_ticks;
    int ipc_wakeup_reason;
};

typedef struct {
    uint64_t process_id;
    address_space_t* addr_space;
    kthread_t* main_thread;

    // Personality tagging for subsystems (e.g., Linux, Android, Windows)
    uint32_t personality;

    // Capability-based security context would be linked here
    void* security_sandbox_ctx;

    // Ownership tracking
    uint32_t owner_core_id;
    uint64_t object_id;
} kprocess_t;

// Scheduler Core
void sched_init(void);

// Create process and main thread
kprocess_t* process_create(const char* name);
int process_destroy(kprocess_t* process);
kthread_t* thread_create(kprocess_t* parent, void (*entry_point)(void));
int thread_destroy(kthread_t* thread);

// Wait Queues
void sched_wait_queue_init(wait_queue_t* queue);
void sched_wait_queue_enqueue(wait_queue_t* queue, kthread_t* thread);
kthread_t* sched_wait_queue_dequeue(wait_queue_t* queue);

// Wait Queue State
void sched_block(void);

// Context Switching
void sched_yield(void);
void sched_on_timer_tick(void);
kthread_t* sched_current_thread(void);
uint64_t sched_get_ticks(void);
void sched_set_policy(sched_policy_t policy);
void sched_reschedule(void);
kthread_t* sched_current(void);
int sched_enqueue(kthread_t* thread, uint32_t core_id);
void sched_sleep(uint64_t millis);
void sched_wakeup(kthread_t* thread);
void sched_wakeup_with_priority(kthread_t* thread, uint32_t wakeup_priority);

// AI governor integration helpers
kthread_t* sched_find_thread_by_id(uint64_t tid);
int sched_set_thread_priority(uint64_t tid, uint32_t new_priority);
int sched_set_thread_preferred_node(uint64_t tid, uint8_t node_id);
int sched_ai_apply_suggestion(const ai_suggestion_t* suggestion);
int sched_enqueue_ai_suggestion(const ai_suggestion_t* suggestion);
int sched_migrate_task(kthread_t* thread, uint32_t new_node);
int sched_adjust_priority(kthread_t* thread, uint32_t new_priority);
int sched_throttle_core(uint32_t core_id);

// System-call style entry points used by trap/syscall layer
int sched_sys_thread_create(kprocess_t* parent, void (*entry_point)(void), uint64_t* out_tid);
int sched_sys_thread_destroy(uint64_t tid);
int sched_sys_sleep(uint64_t millis);
int sched_sys_set_priority(uint64_t tid, uint32_t new_priority);
int sched_sys_set_affinity(uint64_t tid, uint32_t affinity_mask);

// Priority Inheritance support
void sched_inherit_priority(kthread_t* thread, uint32_t new_priority);
void sched_restore_priority(kthread_t* thread);
void sched_on_mutex_wait(kthread_t* waiter, void* mutex);
void sched_on_mutex_acquire(kthread_t* owner, void* mutex);
void sched_on_mutex_release(kthread_t* owner, void* mutex);

// Multikernel IPC integration stub
void sched_notify_ipc_ready(uint32_t core_id, uint32_t msg_type);

#ifdef Profile_RTOS
// Tickless operation for Hard-RT OpenRAN
void sched_disable_tick_for_core(uint32_t core_id);
#endif

#endif // BHARAT_SCHED_H
