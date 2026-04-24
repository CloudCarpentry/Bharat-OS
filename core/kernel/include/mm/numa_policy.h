#ifndef BHARAT_MM_NUMA_POLICY_H
#define BHARAT_MM_NUMA_POLICY_H

#include "../../include/numa.h"
#include "../../include/mm.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Allocate a page according to the given NUMA policy
phys_addr_t mm_alloc_page_policy(const numa_affinity_t *policy);

// Helper for the scheduler or VMM to determine the next interleaving node
memory_node_id_t numa_policy_next_interleave_node(const numa_affinity_t *policy, uint64_t vaddr);

// Helper for scheduler hints
void numa_policy_set_thread_affinity(void *thread, const numa_affinity_t *policy);
int numa_policy_get_thread_affinity(void *thread, numa_affinity_t *out_policy);

#ifdef __cplusplus
}
#endif

#endif // BHARAT_MM_NUMA_POLICY_H
