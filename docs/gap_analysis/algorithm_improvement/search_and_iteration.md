# Search and Iteration Inefficiencies

This document identifies areas in the codebase where inefficient $O(n)$ search algorithms and linear array iterations are used instead of more optimal data structures (like hash maps or trees) or hardware-assisted search mechanisms.

## Subsystems

### 1. Scheduler (`kernel/src/sched.c`)
The scheduler relies heavily on iterating over a global array of threads/processes to find a specific entry by its ID, resulting in $O(n)$ time complexity for thread lookups.

*   **File:** `kernel/src/sched.c`
*   **Functions:**
    *   `sched_find_thread_slot_by_tid`: Iterates sequentially through `g_threads` array to find a thread by `tid`.
    *   `sched_find_free_thread_slot`: Relies on a single linked list head index (`g_free_thread_head`) which is better than $O(n)$, but the allocation pattern could be improved with bitsets or a slab allocator if concurrency scaling is needed.
    *   `sched_find_free_process_slot`: Similar linear or simplistic allocation scheme.
*   **Improvement Suggestion:**
    *   **Algorithmic:** Replace the linear array scan with a Hash Map or a balanced tree (e.g., Red-Black Tree) for $O(1)$ or $O(\log n)$ lookup time by `tid`.
    *   **Algorithmic:** Use bitmap-based allocation (like a buddy allocator or bitset) combined with hardware-accelerated instructions (e.g., Count Trailing Zeros - `CTZ` / `FFS`) to quickly find free thread or process slots in $O(1)$ time.

### 2. Physical Memory Manager - NUMA / Slab (`kernel/src/mm/pmm/numa.c`, `kernel/src/mm/pmm/slab.c`)
*   **File:** `kernel/src/mm/pmm/numa.c`
*   **Context:** Finding or hashing physical pages mapped to NUMA nodes currently uses simplistic list structures or comments indicating "A simple list for PoC is enough".
*   **Improvement Suggestion:**
    *   **Algorithmic:** Implement a proper hash table or radix tree mapping virtual/physical addresses to NUMA nodes instead of a linear list search.

### 3. ZSwap Hash Table (`kernel/src/mm/zswap.c`)
*   **File:** `kernel/src/mm/zswap.c`
*   **Context:** `zswap_hash_table` iteration and simple modulus hashing for page lookups.
*   **Improvement Suggestion:**
    *   **Algorithmic:** Instead of iterating over hash collisions via linked lists, consider a lock-free or concurrent hash map design, or use hardware instructions (like CRC32 or AES instructions, often available in modern CPUs) to compute a stronger and more evenly distributed hash to reduce collisions.

## Summary

The primary bottlenecks are currently inside the core scheduler path (`sched_find_thread_slot_by_tid`). Since scheduler operations happen on every context switch and thread management call, optimizing these $O(n)$ lookups to $O(1)$ hash maps or hardware-assisted bitsets (using `CLZ`/`CTZ` instructions) is a high-priority architectural improvement.
