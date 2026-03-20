#include "../../../include/numa.h"

#include <stddef.h>
#include <stdint.h>

#include "../../../include/hal/hal_pt.h"
#include "../../../include/hal/hal_tlb.h"
#include "../../../include/mm.h"
#include "../../../include/mm/physmap.h"
#include "../../../include/sched.h"
#include "../../../include/slab.h"

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

#define NUMA_MIGRATE_THRESHOLD 100
#define NUMA_MIGRATE_COOLDOWN_TICKS 5000
#define NUMA_ACCESS_HASH_BUCKETS 128U
#define NUMA_PAGE_NODE_HASH_BUCKETS 256U

typedef struct numa_access_record {
    uintptr_t address_space_key;
    uint64_t vpage;
    uint64_t vaddr;
    uint32_t remote_accesses;
    uint64_t last_migrated_tick;
    struct numa_access_record* next;
} numa_access_record_t;

typedef struct numa_page_node_entry {
    uintptr_t address_space_key;
    uint64_t vpage;
    memory_node_id_t node_id;
    struct numa_page_node_entry* next;
} numa_page_node_entry_t;

static kcache_t* numa_record_cache = NULL;
static kcache_t* numa_page_node_cache = NULL;
static numa_access_record_t* g_access_table[NUMA_ACCESS_HASH_BUCKETS];
static numa_page_node_entry_t* g_page_node_table[NUMA_PAGE_NODE_HASH_BUCKETS];
static uint8_t g_numa_tables_initialized = 0U;

static uint64_t hash_mix64(uint64_t value) {
    value ^= value >> 33;
    value *= 0xff51afd7ed558ccdULL;
    value ^= value >> 33;
    value *= 0xc4ceb9fe1a85ec53ULL;
    value ^= value >> 33;
    return value;
}

static uint32_t access_hash_index(uintptr_t address_space_key, uint64_t vpage) {
    uint64_t mixed = hash_mix64(((uint64_t)address_space_key) ^ (vpage << 1));
    return (uint32_t)(mixed & (NUMA_ACCESS_HASH_BUCKETS - 1U));
}

static uint32_t page_node_hash_index(uintptr_t address_space_key, uint64_t vpage) {
    uint64_t mixed = hash_mix64((vpage << 2) ^ ((uint64_t)address_space_key));
    return (uint32_t)(mixed & (NUMA_PAGE_NODE_HASH_BUCKETS - 1U));
}

static void ensure_cache_init() {
    if (!numa_record_cache) {
        numa_record_cache = kcache_create("numa_access_record", sizeof(numa_access_record_t));
    }
    if (!numa_page_node_cache) {
        numa_page_node_cache = kcache_create("numa_page_node_entry", sizeof(numa_page_node_entry_t));
    }
    if (g_numa_tables_initialized == 0U) {
        for (uint32_t i = 0; i < NUMA_ACCESS_HASH_BUCKETS; ++i) {
            g_access_table[i] = NULL;
        }
        for (uint32_t i = 0; i < NUMA_PAGE_NODE_HASH_BUCKETS; ++i) {
            g_page_node_table[i] = NULL;
        }
        g_numa_tables_initialized = 1U;
    }
}

static numa_access_record_t* find_or_create_access_record(uintptr_t address_space_key, uint64_t vaddr) {
    uint64_t vpage = vaddr & ~((uint64_t)PAGE_SIZE - 1ULL);
    uint32_t bucket = access_hash_index(address_space_key, vpage);
    numa_access_record_t* record = g_access_table[bucket];

    while (record) {
        if (record->address_space_key == address_space_key && record->vpage == vpage) {
            return record;
        }
        record = record->next;
    }

    record = (numa_access_record_t*)kcache_alloc(numa_record_cache);
    if (!record) {
        return NULL;
    }

    record->address_space_key = address_space_key;
    record->vpage = vpage;
    record->vaddr = vpage;
    record->remote_accesses = 0U;
    record->last_migrated_tick = 0U;
    record->next = g_access_table[bucket];
    g_access_table[bucket] = record;
    return record;
}

static numa_page_node_entry_t* find_or_create_page_node_entry(uintptr_t address_space_key, uint64_t vaddr) {
    uint64_t vpage = vaddr & ~((uint64_t)PAGE_SIZE - 1ULL);
    uint32_t bucket = page_node_hash_index(address_space_key, vpage);
    numa_page_node_entry_t* entry = g_page_node_table[bucket];

    while (entry) {
        if (entry->address_space_key == address_space_key && entry->vpage == vpage) {
            return entry;
        }
        entry = entry->next;
    }

    entry = (numa_page_node_entry_t*)kcache_alloc(numa_page_node_cache);
    if (!entry) {
        return NULL;
    }

    entry->address_space_key = address_space_key;
    entry->vpage = vpage;
    entry->node_id = NUMA_NODE_LOCAL;
    entry->next = g_page_node_table[bucket];
    g_page_node_table[bucket] = entry;
    return entry;
}

static memory_node_id_t get_mapped_page_node(uintptr_t address_space_key, uint64_t vaddr) {
    numa_page_node_entry_t* entry = find_or_create_page_node_entry(address_space_key, vaddr);
    if (!entry) {
        return NUMA_NODE_LOCAL;
    }
    return entry->node_id;
}

static void set_mapped_page_node(uintptr_t address_space_key, uint64_t vaddr, memory_node_id_t node_id) {
    numa_page_node_entry_t* entry = find_or_create_page_node_entry(address_space_key, vaddr);
    if (entry) {
        entry->node_id = node_id;
    }
}

void numa_record_page_access(void* thread_ptr, uint64_t vaddr, numa_access_type_t access_type) {
    (void)access_type;
    ensure_cache_init();
    kthread_t* thread = (kthread_t*)thread_ptr;
    if (!thread || !thread->process || !thread->process->addr_space) return;

    uintptr_t as_key = (uintptr_t)thread->process->addr_space;
    numa_access_record_t* record = find_or_create_access_record(as_key, vaddr);
    if (!record) {
        return;
    }

    memory_node_id_t page_node = get_mapped_page_node(as_key, vaddr);
    memory_node_id_t current_node = numa_get_current_node();
    if (page_node != current_node) {
        record->remote_accesses++;
    }
}

void numa_select_migration_candidates(void* thread_ptr) {
    ensure_cache_init();
    kthread_t* thread = (kthread_t*)thread_ptr;
    if (!thread || !thread->process || !thread->process->addr_space) return;
    if (thread->preferred_numa_node >= NUMA_MAX_NODES) return;

    uint64_t now_ticks = sched_get_ticks();
    uintptr_t as_key = (uintptr_t)thread->process->addr_space;
    for (uint32_t bucket = 0; bucket < NUMA_ACCESS_HASH_BUCKETS; ++bucket) {
        numa_access_record_t* record = g_access_table[bucket];
        while (record) {
            if (record->address_space_key == as_key &&
                record->remote_accesses >= NUMA_MIGRATE_THRESHOLD &&
                (now_ticks - record->last_migrated_tick) >= NUMA_MIGRATE_COOLDOWN_TICKS) {
                if (numa_migrate_page(record->vaddr, thread->preferred_numa_node, thread->process->addr_space) == 0) {
                    record->last_migrated_tick = now_ticks;
                    record->remote_accesses = 0U;
                }
            }
            record = record->next;
        }
    }
}

int numa_migrate_page(uint64_t vaddr, memory_node_id_t target_node, void* address_space) {
    address_space_t* as = (address_space_t*)address_space;
    if (!as) return -1;
    if (target_node >= NUMA_MAX_NODES) return -1;

    // 1. Allocate a new physical page on target_node
    phys_addr_t new_phys = mm_alloc_pages_order(0, target_node, PAGE_FLAG_USER | PAGE_FLAG_KERNEL);
    if (!new_phys) return -1;

    // 2. Lookup old physical page
    phys_addr_t old_phys = 0;
    uint32_t old_flags = 0;
    if (!active_hal_pt) {
        return -1;
    }
    int ret = active_hal_pt->query_page(as->root_pt, vaddr, &old_phys, &old_flags);
    if (ret < 0 || old_phys == 0) {
        mm_free_page(new_phys);
        return -2;
    }

    // 3. Copy data
    uint8_t* src = (uint8_t*)physmap_phys_to_virt(old_phys);
    uint8_t* dst = (uint8_t*)physmap_phys_to_virt(new_phys);
    for (int i = 0; i < PAGE_SIZE; i++) {
        dst[i] = src[i];
    }

    // 4. Update mapping
    ret = active_hal_pt->unmap_page(as->root_pt, vaddr, NULL);
    if (ret < 0) {
        mm_free_page(new_phys);
        return -3;
    }
    ret = active_hal_pt->map_page(as->root_pt, vaddr, new_phys, old_flags);
    if (ret < 0) {
        mm_free_page(new_phys);
        return -3;
    }

    // 5. Free old page
    mm_free_page(old_phys);

    // 6. TLB shootdown
    tlb_shootdown(as, vaddr);
    set_mapped_page_node((uintptr_t)as, vaddr, target_node);

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
