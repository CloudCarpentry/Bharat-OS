#ifndef BHARAT_MM_PMM_H
#define BHARAT_MM_PMM_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    PMM_ZONE_ANY = 0,
    PMM_ZONE_DMA32,
    PMM_ZONE_NORMAL,
    PMM_ZONE_HIGHMEM,
} pmm_zone_t;

typedef enum {
    PMM_ALLOC_NONE       = 0u,
    PMM_ALLOC_ZERO       = 1u << 0,
    PMM_ALLOC_CONTIGUOUS = 1u << 1,
    PMM_ALLOC_PINNED     = 1u << 2,
} pmm_alloc_flags_t;

typedef struct {
    uint64_t phys_addr;
    uint32_t order;
    uint32_t flags;
} pmm_block_t;

int pmm_alloc_pages(uint32_t order,
                    pmm_zone_t zone,
                    uint32_t alloc_flags,
                    pmm_block_t *out_block);

int pmm_free_pages(const pmm_block_t *block);
int pmm_ref_get(uint64_t phys_addr);
int pmm_ref_put(uint64_t phys_addr);

#ifdef __cplusplus
}
#endif

#endif // BHARAT_MM_PMM_H
