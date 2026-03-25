#ifndef BHARAT_MM_PMM_H
#define BHARAT_MM_PMM_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Forward declaration
struct page;

typedef enum {
    PMM_ZONE_ANY = 0,
    PMM_ZONE_DMA32,
    PMM_ZONE_NORMAL,
    PMM_ZONE_HIGHMEM,
} pmm_zone_t;

typedef enum {
    PMM_PAGE_STATE_FREE = 0,
    PMM_PAGE_STATE_ALLOCATED,
    PMM_PAGE_STATE_RESERVED,
    PMM_PAGE_STATE_DEVICE,
    PMM_PAGE_STATE_OFFLINE,
} pmm_page_state_t;

typedef enum {
    PMM_OWNER_CLASS_NONE = 0,
    PMM_OWNER_CLASS_KERNEL,
    PMM_OWNER_CLASS_USER,
    PMM_OWNER_CLASS_PAGETABLE,
    PMM_OWNER_CLASS_DMA,
    PMM_OWNER_CLASS_DEVICE,
    PMM_OWNER_CLASS_BOOT,
    PMM_OWNER_CLASS_IPC,
    PMM_OWNER_CLASS_CACHE,
} pmm_owner_class_t;

typedef enum {
    PMM_ALLOC_NONE       = 0u,
    PMM_ALLOC_ZERO       = 1u << 0,
    PMM_ALLOC_CONTIGUOUS = 1u << 1,
    PMM_ALLOC_PINNED     = 1u << 2,
} pmm_alloc_flags_t;

typedef enum alloc_class {
    MEM_NORMAL = 0,
    MEM_DMA,
    MEM_RT,
    MEM_SECURE,
    MEM_PACKET,
    MEM_LOWPOWER,
    MEM_PERSISTENT
} alloc_class_t;

typedef struct {
    uint64_t phys_addr;
    uint32_t order;
    uint32_t page_count;
    uint32_t flags;
    alloc_class_t alloc_class;
} pmm_block_t;

int pmm_alloc_pages_ex(uint32_t order,
                       pmm_zone_t zone,
                       alloc_class_t cls,
                       uint32_t alloc_flags,
                       pmm_block_t *out_block);

static inline int pmm_alloc_pages(uint32_t order,
                                  pmm_zone_t zone,
                                  uint32_t alloc_flags,
                                  pmm_block_t *out_block);
static inline int pmm_alloc_pages(uint32_t order,
                                  pmm_zone_t zone,
                                  uint32_t alloc_flags,
                                  pmm_block_t *out_block) {
    return pmm_alloc_pages_ex(order, zone, MEM_NORMAL, alloc_flags, out_block);
}

int pmm_alloc_contiguous_ex(uint32_t page_count,
                            pmm_zone_t zone,
                            alloc_class_t cls,
                            uint32_t alloc_flags,
                            pmm_block_t *out_block);

static inline int pmm_alloc_contiguous(uint32_t page_count,
                                       pmm_zone_t zone,
                                       uint32_t alloc_flags,
                                       pmm_block_t *out_block);
static inline int pmm_alloc_contiguous(uint32_t page_count,
                                       pmm_zone_t zone,
                                       uint32_t alloc_flags,
                                       pmm_block_t *out_block) {
    return pmm_alloc_contiguous_ex(page_count, zone, MEM_NORMAL, alloc_flags, out_block);
}

int pmm_free_pages(const pmm_block_t *block);
int pmm_ref_get(uint64_t phys_addr);
int pmm_ref_put(uint64_t phys_addr);
int pmm_pin(uint64_t phys_addr);
int pmm_unpin(uint64_t phys_addr);

// TODO: Needs refactor: #include directive placed mid-file for dependency/order compatibility.
#include <stdbool.h>

// Helper wrappers
void *pmm_alloc_page_ex(alloc_class_t cls, uint32_t flags);
void *pmm_alloc_zeroed_page_ex(alloc_class_t cls, uint32_t flags);
void *pmm_alloc_contig_ex(size_t npages, size_t align_pages, alloc_class_t cls, uint32_t flags);

static inline void *pmm_alloc_page(uint32_t flags);
static inline void *pmm_alloc_page(uint32_t flags) {
    return pmm_alloc_page_ex(MEM_NORMAL, flags);
}

static inline void *pmm_alloc_zeroed_page(uint32_t flags);
static inline void *pmm_alloc_zeroed_page(uint32_t flags) {
    return pmm_alloc_zeroed_page_ex(MEM_NORMAL, flags);
}

static inline void *pmm_alloc_contig(size_t npages, size_t align_pages, uint32_t flags);
static inline void *pmm_alloc_contig(size_t npages, size_t align_pages, uint32_t flags) {
    return pmm_alloc_contig_ex(npages, align_pages, MEM_NORMAL, flags);
}

void pmm_free_page(void *page);

// Refcount helpers
void page_get(struct page *page);
void page_put(struct page *page);
bool page_try_get(struct page *page);

#ifdef __cplusplus
}
#endif

#endif // BHARAT_MM_PMM_H
