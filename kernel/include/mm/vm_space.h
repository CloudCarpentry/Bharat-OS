#ifndef BHARAT_VM_SPACE_H
#define BHARAT_VM_SPACE_H

#include <stdint.h>
#include <stdbool.h>
#include "../../include/spinlock.h"
#include "../../include/capability.h"
#include "vm_mapping.h"

// Hardware profiles for VM Spaces
typedef enum {
    MEM_PROFILE_MPU_ONLY = 0,
    MEM_PROFILE_MMU_BASIC,
    MEM_PROFILE_MMU_DISTRIBUTED,
    MEM_PROFILE_MMU_DMA_ISOLATED
} mem_profile_t;

// Minimal Core Mask for now
typedef uint64_t cpu_mask_t;

// Realization States
typedef enum {
    VM_REALIZE_NONE = 0,
    VM_REALIZE_PARTIAL,
    VM_REALIZE_VALID,
    VM_REALIZE_RT_VALID
} vm_realize_state_t;

// RT Reserve constraints
typedef struct vm_rt_reserve {
    uint32_t reserved_pt_pages;
    uint32_t reserved_meta_nodes;
    uint32_t reserved_tlb_ops;
} vm_rt_reserve_t;

// Basic tree/db structure for mapping lookups (mocked as simple list for MVP)
typedef struct {
    vm_mapping_t *head;
} vm_mapping_db_t;

typedef struct {
    void *root; // Tree root placeholder
} vm_region_tree_t;

// The canonical distributed address space object
typedef struct vm_space {
    spinlock_t lock;

    uint64_t space_id;
    uint64_t generation;

    mem_profile_t profile;
    vm_timing_class_t timing_class;

    uint32_t flags;
    uint32_t rt_flags;

    cap_handle_t owner_cap;

    vm_region_tree_t regions;
    vm_mapping_db_t mappings;

    cpu_mask_t allowed_cores;
    cpu_mask_t active_cores;
    cpu_mask_t realized_cores;
    cpu_mask_t pending_cores;
    cpu_mask_t rt_ready_cores;

    uint32_t home_monitor;

    vm_rt_reserve_t rt_reserve;

    bool require_prefault;
    bool allow_lazy_realize;
    bool allow_runtime_pt_alloc;
    bool allow_remote_fault_recovery;
    bool allow_demand_paging;
} vm_space_t;

// Per-core realization state
typedef struct vm_core_state {
    uint32_t core_id;

    uint64_t realized_generation;
    uint64_t tlb_generation;

    uintptr_t root_pa;
    void *root_va;

    uint32_t asid_or_pcid;
    vm_realize_state_t state;

    uint32_t flags;
} vm_core_state_t;

// Current VM state on a core
typedef struct core_vm_state {
    vm_space_t *current_space;
    uint64_t current_generation;
    uintptr_t current_root_pa;
    uint32_t current_asid_or_pcid;
} core_vm_state_t;

// Core VM APIs
int vm_space_create(vm_space_t **out, mem_profile_t profile, vm_timing_class_t timing);
int vm_space_destroy(vm_space_t *space);

int vm_map(vm_space_t *space, const vm_map_req_t *req);
int vm_unmap(vm_space_t *space, uintptr_t va, size_t len);
int vm_protect(vm_space_t *space, uintptr_t va, size_t len, uint64_t prot, uint64_t mem_type);

int vm_realize_on_core(vm_space_t *space, uint32_t core_id, bool strict);
int vm_prepare_rt_core(vm_space_t *space, uint32_t core_id);
int vm_activate_local(vm_space_t *space);

#endif // BHARAT_VM_SPACE_H
