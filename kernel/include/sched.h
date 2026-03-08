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
    THREAD_STATE_TERMINATED
} thread_state_t;

typedef enum {
    SCHED_POLICY_ROUND_ROBIN = 0,
    SCHED_POLICY_CLOUD_FAIR  = 1,
    SCHED_POLICY_PRIORITY = 2,
    SCHED_POLICY_EDF = 3
} sched_policy_t;

typedef enum {
    PERSONALITY_NATIVE = 0,
    PERSONALITY_LINUX = 1
} personality_type_t;


#define SCHED_MAX_PRIORITY 31U

typedef struct {
    uint64_t regs[16];
    uint64_t pc;
    uint64_t sp;
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
    uint64_t total_ticks;
    uint32_t throttled;
} sched_core_t;

struct kthread {
    uint64_t thread_id;
    uint64_t process_id;

    // Personality type for ABI compatibility
    personality_type_t personality;

    // CPU Architectural Context (Registers)
    void* cpu_context;

    // Kernel Stack
    virt_addr_t kernel_stack;

    thread_state_t state;
    uint32_t priority;

    // Priority Inheritance (Hard-RT / OpenRAN profile)
    uint32_t base_priority;
    void* waiting_on_lock; // Mutex the thread is waiting for

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
};

typedef struct {
    uint64_t process_id;
    address_space_t* addr_space;
    kthread_t* main_thread;

    // Capability-based security context would be linked here
    void* security_sandbox_ctx;
} kprocess_t;

// Scheduler Core
void sched_init(void);

// Create process and main thread
kprocess_t* process_create(const char* name);
kthread_t* thread_create(kprocess_t* parent, void (*entry_point)(void));
int thread_destroy(kthread_t* thread);

// Context Switching
void sched_yield(void);
void sched_on_timer_tick(void);
kthread_t* sched_current_thread(void);
uint64_t sched_get_ticks(void);
void sched_set_policy(sched_policy_t policy);
void sched_sleep(uint64_t millis);
void sched_wakeup(kthread_t* thread);

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

// Priority Inheritance support
void sched_inherit_priority(kthread_t* thread, uint32_t new_priority);
void sched_restore_priority(kthread_t* thread);

// Multikernel IPC integration stub
void sched_notify_ipc_ready(uint32_t core_id, uint32_t msg_type);

#ifdef Profile_RTOS
// Tickless operation for Hard-RT OpenRAN
void sched_disable_tick_for_core(uint32_t core_id);
#endif

#endif // BHARAT_SCHED_H
