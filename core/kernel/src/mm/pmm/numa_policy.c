#include "../../include/mm/numa_policy.h"
#include "../../include/numa.h"
#include "../../include/mm.h"
#include "sched/sched.h"

phys_addr_t mm_alloc_page_policy(const numa_affinity_t *policy) {
    return mm_alloc_page_policy_at(policy, 0U);
}

extern phys_addr_t pmm_alloc_page_node(memory_node_id_t node, int strict);

phys_addr_t mm_alloc_page_policy_at(const numa_affinity_t *policy, uint64_t vaddr) {
    if (!policy) {
        return mm_alloc_page(NUMA_NODE_ANY);
    }

    if (policy->policy == NUMA_POLICY_BIND) {
        return pmm_alloc_page_node(policy->target_node, 1);
    }
    else if (policy->policy == NUMA_POLICY_LOCAL_PREFERRED) {
        return pmm_alloc_page_node(policy->target_node, 0);
    }
    else if (policy->policy == NUMA_POLICY_INTERLEAVE) {
        memory_node_id_t node = numa_policy_next_interleave_node(policy, vaddr);
        if (node == NUMA_NODE_ANY) return 0;
        return pmm_alloc_page_node(node, 1);
    }

    return mm_alloc_page(NUMA_NODE_ANY);
}

memory_node_id_t numa_policy_next_interleave_node(const numa_affinity_t *policy, uint64_t vaddr) {
    if (!policy || policy->policy != NUMA_POLICY_INTERLEAVE || policy->interleave_mask == 0) {
        return NUMA_NODE_ANY;
    }

    // Typical interleave size is 4KB or 2MB. We use 4KB chunks.
    // Hash or simply mod the page frame number to distribute across the bitmask.
    uint64_t pfn = vaddr >> 12;

    // Count how many nodes are set in the mask
    int node_count = 0;
    int nodes[16];
    for (int i = 0; i < 16; i++) {
        if (policy->interleave_mask & (1 << i)) {
            nodes[node_count++] = i;
        }
    }

    if (node_count == 0) return NUMA_NODE_ANY;

    int selected_idx = pfn % node_count;
    return nodes[selected_idx];
}

// Scheduler hints (stub implementation)
void numa_policy_set_thread_affinity(void *thread, const numa_affinity_t *policy) {
    if (!thread || !policy) return;
    bh_thread_t *t = (bh_thread_t *)thread;
    t->numa_affinity = *policy;
}

int numa_policy_get_thread_affinity(void *thread, numa_affinity_t *out_policy) {
    if (!thread || !out_policy) return -1;
    bh_thread_t *t = (bh_thread_t *)thread;

    *out_policy = t->numa_affinity;
    return 0;
}
