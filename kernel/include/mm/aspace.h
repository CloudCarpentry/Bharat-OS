#ifndef BHARAT_MM_ASPACE_H
#define BHARAT_MM_ASPACE_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "vm_object.h"
#include "../../include/spinlock.h"
#include "../../include/numa.h"
#include "../../include/mm.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct vm_region {
    uint64_t base;
    uint64_t length;
    uint32_t prot;
    uint32_t map_flags;
    vm_object_t *object;
    uint64_t object_offset;

    numa_affinity_t numa_policy;

    // Region tree links (using simple intrusive linked list or RB-tree scaffold)
    // For now, intrusive linked list sorted by base address.
    struct vm_region *next;
    struct vm_region *prev;
} vm_region_t;

typedef struct vm_address_space {
    uint64_t object_id;      // Unique ID compatible with legacy object_id tracking
    uint32_t owner_core_id;  // Owner core compatible with legacy tracking
    phys_addr_t root_pt;     // Hardware Page-Table Root (hal_pt compatible)
    spinlock_t lock;         // Serialization lock

    volatile uint64_t tlb_seq; // Monotonically increasing sequence for TLB shootdowns

    vm_region_t *regions;    // Head of region tree / list

    uint64_t user_base;      // Min user VA
    uint64_t user_limit;     // Max user VA
} vm_address_space_t;

// Lifecycle
int aspace_create(vm_address_space_t **out_aspace);
int aspace_destroy(vm_address_space_t *aspace);

// Region reservation and mapping
int aspace_map_region(vm_address_space_t *aspace, uint64_t vaddr_hint, uint64_t length, uint32_t prot, uint32_t map_flags, vm_object_t *object, uint64_t object_offset, uint64_t *out_vaddr);

// Unmap and protect
int aspace_unmap_region(vm_address_space_t *aspace, uint64_t base, uint64_t length);
int aspace_protect_region(vm_address_space_t *aspace, uint64_t base, uint64_t length, uint32_t new_prot);

// Authoritative VA lookup
int aspace_find_region(vm_address_space_t *aspace, uint64_t vaddr, vm_region_t **out_region);

// Utility: overlap check (mostly internal but exposed for testing)
bool aspace_check_overlap(vm_address_space_t *aspace, uint64_t base, uint64_t length);

#ifdef __cplusplus
}
#endif

#endif // BHARAT_MM_ASPACE_H
