#include "../../include/mm/numa_policy.h"
#include "../../include/numa.h"
#include "../../include/mm.h"
#include "sched/sched.h"

phys_addr_t mm_alloc_page_policy(const numa_affinity_t *policy) {
    if (!policy) {
        // Fallback to ANY
        return mm_alloc_page(NUMA_NODE_ANY);
    }

    if (policy->policy == NUMA_POLICY_BIND) {
        // Try strict bind first. If it fails, we return 0 (OOM for this node).
        phys_addr_t pa = mm_alloc_page(policy->target_node);
        return pa;
    }
    else if (policy->policy == NUMA_POLICY_LOCAL_PREFERRED) {
        phys_addr_t pa = mm_alloc_page(policy->target_node);
        if (pa == 0) {
            // Fallback to any node if local is full
            pa = mm_alloc_page(NUMA_NODE_ANY);
        }
        return pa;
    }
    else if (policy->policy == NUMA_POLICY_INTERLEAVE) {
        // The caller must figure out the interleave node and pass it as target_node
        // because interleave is based on virtual address block alignment.
        // We assume target_node was populated by numa_policy_next_interleave_node.
        phys_addr_t pa = mm_alloc_page(policy->target_node);
        if (pa == 0) {
            pa = mm_alloc_page(NUMA_NODE_ANY); // Fallback to avoid crashing
        }
        return pa;
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
    t->preferred_numa_node = policy->target_node;
}

int numa_policy_get_thread_affinity(void *thread, numa_affinity_t *out_policy) {
    if (!thread || !out_policy) return -1;
    bh_thread_t *t = (bh_thread_t *)thread;

    out_policy->policy = NUMA_POLICY_LOCAL_PREFERRED;
    out_policy->target_node = t->preferred_numa_node;
    return 0;
}
