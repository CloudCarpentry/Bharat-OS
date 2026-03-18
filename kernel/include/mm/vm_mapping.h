#ifndef BHARAT_VM_MAPPING_H
#define BHARAT_VM_MAPPING_H

#include <stdint.h>
#include <stddef.h>
#include "../../include/capability.h"

typedef enum {
    VM_MEM_ZONE_RT_LOCAL = 0,
    VM_MEM_ZONE_NORMAL,
    VM_MEM_ZONE_SHARED,
    VM_MEM_ZONE_DEVICE,
    VM_MEM_ZONE_RECLAIMABLE
} vm_mem_zone_t;

// Timing Classes
typedef enum {
    VM_TIMING_BEST_EFFORT = 0,
    VM_TIMING_SOFT_RT,
    VM_TIMING_FIRM_RT,
    VM_TIMING_HARD_RT
} vm_timing_class_t;

// Mapping Flags
#define VM_MAP_PREFAULT        (1ULL << 0)
#define VM_MAP_PINNED          (1ULL << 1)
#define VM_MAP_LOCKED          (1ULL << 2)
#define VM_MAP_NO_LAZY_SYNC    (1ULL << 3)
#define VM_MAP_EXEC_OK         (1ULL << 4)
#define VM_MAP_DEVICE          (1ULL << 5)
#define VM_MAP_GLOBAL          (1ULL << 6)
#define VM_MAP_RT_CRITICAL     (1ULL << 7)

// Common arch-agnostic mapping permissions
#define VM_PROT_READ    (1ULL << 0)
#define VM_PROT_WRITE   (1ULL << 1)
#define VM_PROT_EXEC    (1ULL << 2)
#define VM_PROT_USER    (1ULL << 3)

// Common memory types
#define VM_MEM_NORMAL       0
#define VM_MEM_DEVICE       1
#define VM_MEM_UNCACHED     2
#define VM_MEM_WRITE_COMB   3

typedef struct vm_mapping {
    uintptr_t va_start;
    uintptr_t pa_start;
    size_t length;

    uint64_t prot;
    uint64_t mem_type;
    uint64_t flags;

    vm_mem_zone_t zone;
    cap_handle_t backing_cap;

    uint64_t map_gen;
    struct vm_mapping *next;
    struct vm_mapping *prev;
} vm_mapping_t;

typedef struct vm_map_req {
    uintptr_t va;
    uintptr_t pa;
    size_t len;

    uint64_t prot;
    uint64_t mem_type;
    uint64_t map_flags;

    vm_mem_zone_t zone;
    vm_timing_class_t timing_class;
} vm_map_req_t;

#endif // BHARAT_VM_MAPPING_H
