#ifndef BHARAT_HAL_HAL_PT_H
#define BHARAT_HAL_HAL_PT_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t read       : 1;
    uint32_t write      : 1;
    uint32_t exec       : 1;
    uint32_t user       : 1;
    uint32_t global     : 1;
    uint32_t device     : 1;
    uint32_t uncached   : 1;
    uint32_t huge       : 1;
    uint32_t cow        : 1;
    uint32_t shareable  : 1;
    uint32_t reserved   : 22;
} hal_pt_perms_t;

typedef struct {
    uint64_t phys_addr;
    hal_pt_perms_t perms;
    uint64_t page_size;
} hal_pt_mapping_t;

typedef struct {
    uint64_t root;
    uint16_t asid;
} hal_pt_aspace_t;

int hal_pt_aspace_create(hal_pt_aspace_t *out_aspace);
int hal_pt_aspace_destroy(const hal_pt_aspace_t *aspace);
int hal_pt_aspace_activate(const hal_pt_aspace_t *aspace);

int hal_pt_map(const hal_pt_aspace_t *aspace,
               uint64_t virt_addr,
               uint64_t phys_addr,
               uint64_t length,
               hal_pt_perms_t perms);

int hal_pt_unmap(const hal_pt_aspace_t *aspace,
                 uint64_t virt_addr,
                 uint64_t length);

int hal_pt_protect(const hal_pt_aspace_t *aspace,
                   uint64_t virt_addr,
                   uint64_t length,
                   hal_pt_perms_t perms);

int hal_pt_query(const hal_pt_aspace_t *aspace,
                 uint64_t virt_addr,
                 hal_pt_mapping_t *out_mapping);

#ifdef __cplusplus
}
#endif

#endif // BHARAT_HAL_HAL_PT_H
