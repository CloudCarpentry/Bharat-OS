#ifndef BHARAT_MM_ASPACE_H
#define BHARAT_MM_ASPACE_H

#include <stddef.h>
#include <stdint.h>

#include "vm_object.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct vm_address_space vm_address_space_t;

typedef struct {
    uint64_t base;
    uint64_t length;
    uint32_t prot;
    uint32_t map_flags;
    vm_object_t *object;
    uint64_t object_offset;
} vm_region_desc_t;

int aspace_create(vm_address_space_t **out_aspace);
int aspace_destroy(vm_address_space_t *aspace);

int aspace_map_region(vm_address_space_t *aspace, const vm_region_desc_t *desc);
int aspace_unmap_region(vm_address_space_t *aspace, uint64_t base, uint64_t length);
int aspace_protect_region(vm_address_space_t *aspace, uint64_t base, uint64_t length, uint32_t new_prot);

int aspace_find_region(vm_address_space_t *aspace, uint64_t vaddr, vm_region_desc_t *out_desc);

#ifdef __cplusplus
}
#endif

#endif // BHARAT_MM_ASPACE_H
