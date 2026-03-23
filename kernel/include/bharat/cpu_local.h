#ifndef BHARAT_CPU_LOCAL_H
#define BHARAT_CPU_LOCAL_H

#include <stdint.h>
#include <stddef.h>

/* Forward declarations */
struct sched_rq;
struct capability_table;
struct pmm_shard;
struct urpc_port;
struct kthread;

#define MAX_CPUS 32U

/* Opaque forward declarations for specific subsystems.
 * Real definitions will be in their respective headers.
 */

// We will use existing structs but forward declare or include them here.
// In Bharat-OS sched.c uses core_runqueue_t as the per core queue
// TODO: Needs refactor: #include directive placed mid-file for dependency/order compatibility.
#include "capability.h"
// TODO: Needs refactor: #include directive placed mid-file for dependency/order compatibility.
#include "sched.h"
// #include <bharat/pmm.h>
// #include <bharat/urpc.h>

// For now, let's use void* for runqueue if we don't expose core_runqueue_t in public headers,
// or we will rely on subsystem changes (e.g. sched.c defines its own internal struct but exposes a pointer).
// Actually, we can move `core_runqueue_t` to a private header or define it in sched.c and store a pointer here.
// Better yet, since CPU local struct needs the size to be inline (for performance without pointer indirection),
// we will have to expose core_runqueue_t.

// Let's create an opaque pointer array or simply include the right headers once we figure out where things live.

/* In a real implementation we define this structure */
typedef struct cpu_local {
    uint32_t          cpu_id;
    // We will place a pointer for now, or the struct itself if we can refactor sched.h.
    sched_rq_t        runqueue;       // per-core, no lock needed
    capability_table_t cap_table;     // per-core capability namespace
    void             *pmm;            // struct pmm_shard *
    void             *urpc_ports[MAX_CPUS]; // struct urpc_port *
    struct kthread   *current;        // currently running thread
    struct kthread   *idle;           // per-core idle thread
    uintptr_t         kernel_stack;

    // Active address space
    uint64_t          current_as_id;
    address_space_t  *current_as;
} cpu_local_t;

#define KERNEL_AS_ID 0

// Provide an array of locals for all CPUs.
extern cpu_local_t g_cpu_locals[MAX_CPUS];

void cpu_local_init(uint32_t cpu_id);

/* Architecture specific `this_cpu()` implementation */
static inline cpu_local_t *this_cpu(void) {
#if defined(__x86_64__)
    cpu_local_t *cl;
    // Assume we've written the address into GSBASE
    __asm__ volatile ("movq %%gs:0, %0" : "=r"(cl));
    return cl;
#elif defined(__aarch64__)
    cpu_local_t *cl;
    __asm__ volatile ("mrs %0, tpidr_el1" : "=r"(cl));
    return cl;
#elif defined(__riscv)
    cpu_local_t *cl;
    __asm__ volatile ("mv %0, tp" : "=r"(cl));
    return cl;
#else
    // Fallback: This is not safe unless single core, but helps compilation.
    // For a real system we must use the arch's thread pointer register.
    // Currently, let's assume we implement hal_get_cpu_id() and use the array.
    extern uint32_t hal_get_cpu_id(void);
    return &g_cpu_locals[hal_get_cpu_id()];
#endif
}

// Helpers for the current address space
static inline uint64_t core_current_as_id(void) {
    cpu_local_t* cpu = this_cpu();
    return cpu ? cpu->current_as_id : KERNEL_AS_ID;
}

#endif // BHARAT_CPU_LOCAL_H
