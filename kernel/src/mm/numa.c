#include "../../include/numa.h"

#include <stddef.h>

static numa_node_descriptor_t g_nodes[NUMA_MAX_NODES];
static uint32_t g_active_nodes;

int numa_discover_topology(void) {
    // Baseline: single NUMA node until ACPI SRAT/FDT parsing lands.
    g_nodes[0].node_id = 0U;
    g_nodes[0].start_addr = 0U;
    g_nodes[0].size_bytes = 0U;
    g_nodes[0].cpu_count = 1U;
    g_nodes[0].active = 1U;

    for (uint32_t i = 1U; i < NUMA_MAX_NODES; ++i) {
        g_nodes[i].active = 0U;
    }

    g_active_nodes = 1U;
    return 0;
}

memory_node_id_t numa_get_current_node(void) {
    return NUMA_NODE_LOCAL;
}

int numa_set_node_descriptor(memory_node_id_t node_id,
                             uint64_t start_addr,
                             uint64_t size_bytes,
                             uint32_t cpu_count) {
    if (node_id >= NUMA_MAX_NODES || cpu_count == 0U) {
        return -1;
    }

    g_nodes[node_id].node_id = node_id;
    g_nodes[node_id].start_addr = start_addr;
    g_nodes[node_id].size_bytes = size_bytes;
    g_nodes[node_id].cpu_count = cpu_count;
    g_nodes[node_id].active = 1U;

    if ((uint32_t)(node_id + 1U) > g_active_nodes) {
        g_active_nodes = (uint32_t)(node_id + 1U);
    }

    return 0;
}

int numa_get_node_descriptor(memory_node_id_t node_id, numa_node_descriptor_t* out_desc) {
    if (!out_desc || node_id >= NUMA_MAX_NODES || g_nodes[node_id].active == 0U) {
        return -1;
    }

    *out_desc = g_nodes[node_id];
    return 0;
}

uint32_t numa_active_node_count(void) {
    return g_active_nodes;
}

#include "../../include/sched.h"
#include "../../include/slab.h"
#include "../../include/hal/vmm.h"

#define P2V(x) ((void*)(uintptr_t)(x))
#define V2P(x) ((phys_addr_t)(uintptr_t)(x))

#define NUMA_MIGRATE_THRESHOLD 100
#define NUMA_MIGRATE_COOLDOWN_TICKS 5000

typedef struct numa_access_record {
    uint64_t vaddr;
    uint32_t remote_accesses;
    uint64_t last_migrated_tick;
    list_head_t list;
} numa_access_record_t;

// Assuming some thread-local access records, we'd store a list of these in kthread_t,
// but for simplicity we'll just implement the structure and functions here.
static kcache_t* numa_record_cache = NULL;

static void ensure_cache_init() {
    if (!numa_record_cache) {
        numa_record_cache = kcache_create("numa_access_record", sizeof(numa_access_record_t));
    }
}

// In a real system, we might hash vaddr, but a simple list for PoC is enough.
// Alternatively, embed the `numa_access_record_t` inside `kthread_t` or an associated struct.

// Mock global ticks
extern uint64_t g_sched_ticks;

void numa_record_page_access(void* thread_ptr, uint64_t vaddr, numa_access_type_t access_type) {
    (void)access_type;
    ensure_cache_init();
    kthread_t* thread = (kthread_t*)thread_ptr;
    if (!thread) return;

    // For PoC: simulate recording by updating some structure or finding it
    // In a real implementation, this list would be inside `kthread_t`
    // e.g. thread->numa_records
    // We'll just assume there is a list we can iterate.
    // For now, this is a stub as per "software-driven NUMA migration framework".
}

void numa_select_migration_candidates(void* thread_ptr) {
    kthread_t* thread = (kthread_t*)thread_ptr;
    if (!thread) return;

    // Iterate through thread's recorded access pages.
    // If a page's remote_accesses > NUMA_MIGRATE_THRESHOLD and
    // it's off cooldown, queue it for migration.
}

int numa_migrate_page(uint64_t vaddr, memory_node_id_t target_node, void* address_space) {
    address_space_t* as = (address_space_t*)address_space;
    if (!as) return -1;

    // 1. Allocate a new physical page on target_node
    phys_addr_t new_phys = mm_alloc_pages_order(0, target_node, PAGE_FLAG_USER | PAGE_FLAG_KERNEL);
    if (!new_phys) return -1;

    // 2. Lookup old physical page
    phys_addr_t old_phys = 0;
    uint32_t old_flags = 0;
    int ret = hal_vmm_get_mapping(as->root_table, vaddr, &old_phys, &old_flags);
    if (ret < 0 || old_phys == 0) {
        mm_free_page(new_phys);
        return -2;
    }

    // 3. Copy data
    uint8_t* src = (uint8_t*)P2V(old_phys);
    uint8_t* dst = (uint8_t*)P2V(new_phys);
    for (int i = 0; i < PAGE_SIZE; i++) {
        dst[i] = src[i];
    }

    // 4. Update mapping
    ret = hal_vmm_update_mapping(as->root_table, vaddr, new_phys, old_flags);
    if (ret < 0) {
        mm_free_page(new_phys);
        return -3;
    }

    // 5. Free old page
    mm_free_page(old_phys);

    // 6. TLB shootdown
    tlb_shootdown(vaddr);

    return 0;
}

void numa_balance_thread_memory(void* thread_ptr) {
    kthread_t* thread = (kthread_t*)thread_ptr;
    if (!thread) return;

    // In a background worker, run `numa_select_migration_candidates`
    // and then call `numa_migrate_page` on candidates.
    numa_select_migration_candidates(thread);
    // ... migration loop here ...
}
