#ifndef BHARAT_SCHED_H
#define BHARAT_SCHED_H

#include <stdint.h>
#include "mm.h"

/*
 * Bharat-OS Process & Thread Management
 * Handles contexts from RTOS edge threading to datacenter high-throughput workloads.
 */

typedef enum {
    THREAD_STATE_READY,
    THREAD_STATE_RUNNING,
    THREAD_STATE_BLOCKED,
    THREAD_STATE_TERMINATED
} thread_state_t;

typedef struct {
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

    // Time slicing metadata
    uint64_t time_slice_ms;
    uint64_t cpu_time_consumed;
} kthread_t;

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

// Context Switching
void sched_yield(void);

// Priority Inheritance support
void sched_inherit_priority(kthread_t* thread, uint32_t new_priority);
void sched_restore_priority(kthread_t* thread);

#ifdef Profile_RTOS
// Tickless operation for Hard-RT OpenRAN
void sched_disable_tick_for_core(uint32_t core_id);
#endif

#endif // BHARAT_SCHED_H
