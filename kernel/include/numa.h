#ifndef BHARAT_OS_NUMA_H
#define BHARAT_OS_NUMA_H

#include <stdint.h>
#include <stddef.h>

/**
 * NUMA-Ready Interface Definitions (ADR-006)
 */

typedef uint16_t memory_node_id_t;

#define NUMA_NODE_LOCAL 0
#define NUMA_NODE_ANY   0xFFFF
#define NUMA_MAX_NODES 8

typedef struct {
    memory_node_id_t node_a;
    memory_node_id_t node_b;
    uint32_t distance_cost;
} numa_distance_t;

typedef struct {
    memory_node_id_t preferred_node;
    uint8_t strict_bind;
} numa_affinity_t;

typedef struct {
    memory_node_id_t node_id;
    uint64_t start_addr;
    uint64_t size_bytes;
    uint32_t cpu_count;
    uint8_t active;
} numa_node_descriptor_t;

int numa_discover_topology();
memory_node_id_t numa_get_current_node();

int numa_set_node_descriptor(memory_node_id_t node_id,
                             uint64_t start_addr,
                             uint64_t size_bytes,
                             uint32_t cpu_count);
int numa_get_node_descriptor(memory_node_id_t node_id, numa_node_descriptor_t* out_desc);
uint32_t numa_active_node_count(void);

// Deferred Page Migration & TLB Monitoring (NUMA-Aware Page Migration)
typedef enum {
    NUMA_ACCESS_READ = 0,
    NUMA_ACCESS_WRITE = 1,
    NUMA_ACCESS_EXECUTE = 2
} numa_access_type_t;

// To be called from page fault handler or TLB miss profiler
void numa_record_page_access(void* thread, uint64_t vaddr, numa_access_type_t access_type);

// To be called from a background worker thread or periodic scheduler hook
void numa_select_migration_candidates(void* thread);
void numa_balance_thread_memory(void* thread);

// Core migration execution (called by worker)
int numa_migrate_page(uint64_t vaddr, memory_node_id_t target_node, void* address_space);

#endif // BHARAT_OS_NUMA_H
