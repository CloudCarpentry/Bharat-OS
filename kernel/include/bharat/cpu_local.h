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
#include "sched/sched.h"
// #include <bharat/pmm.h>
// #include <bharat/urpc.h>

// For now, let's use void* for runqueue if we don't expose core_runqueue_t in public headers,
// or we will rely on subsystem changes (e.g. sched.c defines its own internal struct but exposes a pointer).
// Actually, we can move `core_runqueue_t` to a private header or define it in sched.c and store a pointer here.
// Better yet, since CPU local struct needs the size to be inline (for performance without pointer indirection),
// we will have to expose core_runqueue_t.

// Let's create an opaque pointer array or simply include the right headers once we figure out where things live.

/* In a real implementation we define this structure */
typedef struct __attribute__((aligned(64))) cpu_local {
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

    // CPU online state
    bool              is_online;
} cpu_local_t;

#define KERNEL_AS_ID 0

// Provide an array of locals for all CPUs.
extern cpu_local_t g_cpu_locals[MAX_CPUS] __attribute__((aligned(64)));

void cpu_local_init(uint32_t cpu_id);

#include <hal/cpu_local.h>

/* Architecture specific `this_cpu()` implementation */
static inline cpu_local_t *this_cpu(void) {
    return hal_cpu_local_ptr();
}

// Helpers for the current address space
static inline uint64_t core_current_as_id(void) {
    cpu_local_t* cpu = this_cpu();
    return cpu ? cpu->current_as_id : KERNEL_AS_ID;
}

#endif // BHARAT_CPU_LOCAL_H

/* Architecture-specific implementation for setting the current CPU local pointer */
void arch_cpu_local_set(cpu_local_t *cl);
