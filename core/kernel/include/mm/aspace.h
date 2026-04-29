#ifndef BHARAT_MM_ASPACE_H
#define BHARAT_MM_ASPACE_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "vm_object.h"
#include "../../include/spinlock.h"
#include "../../include/numa.h"
#include "../../include/mm.h"
#include "kernel/status.h"

#ifdef __cplusplus
extern "C" {
#endif

#define VM_REGION_FLAG_STACK (1 << 0)
#define VM_REGION_FLAG_COW   (1 << 1)

typedef enum {
    ASPACE_STATE_CREATED = 0,
    ASPACE_STATE_ACTIVE,
    ASPACE_STATE_DYING,
    ASPACE_STATE_DESTROYED
} aspace_state_t;

typedef struct vm_region {
    uintptr_t base;
    size_t length;

    uint32_t prot;
    uint32_t map_flags;
    uint32_t region_flags;
    vm_inherit_t inherit;

    uint64_t object_offset;

    struct vm_object *object;

    // RB-Tree linkage
    struct vm_region *left;
    struct vm_region *right;
    struct vm_region *parent;
    uint32_t color;      // 0 for black, 1 for red
    uintptr_t max_end;   // Augmented: max end address in this subtree

    struct vm_region *next; // Retain list linkage for fast sequential iteration
    struct vm_region *prev;
} vm_region_t;

// TODO: Needs refactor: #include directive placed mid-file for dependency/order compatibility.
#include "prot_domain.h"

typedef struct vm_address_space {
    uint64_t object_id;      // Unique ID compatible with legacy object_id tracking
    uint32_t owner_core_id;  // Owner core compatible with legacy tracking

    prot_domain_t* prot_domain; // New generic Protection Domain Backends Layer
    phys_addr_t root_pt;     // Legacy (to remove)

    spinlock_t lock;         // Serialization lock

    volatile uint64_t tlb_gen;       // Epoch generation for TLB shootdowns
    volatile uint64_t active_mask;   // Bitmask of CPUs currently running this aspace

    vm_region_t *regions_tree_root; // Root of the augmented RB-tree
    vm_region_t *regions;           // Head of region list for iteration
    size_t region_count;

    uint64_t user_base;      // Min user VA
    uint64_t user_limit;     // Max user VA

    uint32_t flags;
    aspace_state_t state;
    void *owner;             // process/container owner pointer

    uint32_t timing_class;   // From vm_timing_class_t
} vm_address_space_t;

typedef vm_address_space_t address_space_t;

// Lifecycle
int aspace_create(address_space_t **out_aspace, uint32_t flags);
int aspace_destroy(address_space_t *aspace);
int aspace_clone(address_space_t *src, address_space_t **out_clone, uint32_t clone_flags);

// Region reservation and mapping
int aspace_region_reserve(address_space_t *aspace,
                          uintptr_t base,
                          size_t length,
                          uint32_t prot,
                          uint32_t map_flags,
                          vm_inherit_t inherit,
                          vm_region_t **out_region);

int aspace_region_attach(address_space_t *aspace,
                         uintptr_t base,
                         size_t length,
                         uint32_t prot,
                         uint32_t map_flags,
                         vm_inherit_t inherit,
                         vm_object_t *object,
                         uint64_t object_offset,
                         vm_region_t **out_region);

int aspace_region_detach(address_space_t *aspace, uintptr_t base);

// Unmap and protect (Legacy placeholders for now)
int aspace_map_region(vm_address_space_t *aspace, uint64_t vaddr_hint, uint64_t length, uint32_t prot, uint32_t map_flags, vm_object_t *object, uint64_t object_offset, uint64_t *out_vaddr);
int aspace_unmap_region(vm_address_space_t *aspace, uint64_t base, uint64_t length);
int aspace_protect_region(vm_address_space_t *aspace, uint64_t base, uint64_t length, uint32_t new_prot);
int aspace_find_region(vm_address_space_t *aspace, uint64_t vaddr, vm_region_t **out_region);

// Authoritative VA lookup
vm_region_t *aspace_lookup_region(address_space_t *aspace, uintptr_t va);
vm_object_t *aspace_lookup_object(address_space_t *aspace, uintptr_t va, vm_region_t **out_region, uint64_t *out_object_offset);

// Lifecycle hardening APIs
kstatus_t aspace_activate_on_cpu(address_space_t *aspace, uint32_t cpu_id);
kstatus_t aspace_deactivate_on_cpu(address_space_t *aspace, uint32_t cpu_id);
uint64_t  aspace_get_active_mask(address_space_t *aspace);
uint64_t  aspace_next_tlb_generation(address_space_t *aspace);
bool      aspace_is_valid_for_tlb(address_space_t *aspace);

// Utility: overlap check (mostly internal but exposed for testing)
bool aspace_check_overlap(address_space_t *aspace, uint64_t base, uint64_t length);

#ifdef __cplusplus
}
#endif

#endif // BHARAT_MM_ASPACE_H
