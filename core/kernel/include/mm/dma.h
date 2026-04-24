#ifndef BHARAT_MM_DMA_H
#define BHARAT_MM_DMA_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    DMA_MAP_TO_DEVICE = 0,
    DMA_MAP_FROM_DEVICE,
    DMA_MAP_BIDIRECTIONAL,
} dma_direction_t;

typedef enum {
    DMA_ALLOC_NONE      = 0u,
    DMA_ALLOC_COHERENT  = 1u << 0,
    DMA_ALLOC_DMA32     = 1u << 1,
    DMA_ALLOC_ZERO      = 1u << 2,
} dma_alloc_flags_t;

// TODO: Needs refactor: #include directive placed mid-file for dependency/order compatibility.
#include "../../include/spinlock.h"
// TODO: Needs refactor: #include directive placed mid-file for dependency/order compatibility.
#include "mm/iommu.h"

// Forward declaration of IOMMU backend ops
typedef struct iommu_ops iommu_ops_t;

// IOVA Domain (I/O Virtual Address Space)
typedef struct iova_domain {
    uint64_t id;
    uint64_t base;
    uint64_t limit;
    spinlock_t lock;
    void *allocator_data; // Bitmap or tree backend for finding free ranges

    const iommu_ops_t *iommu; // Binding to physical IOMMU backend (VT-d, SMMU, etc)
    void *iommu_hw_state;     // Hardware context table pointer
    iommu_domain_t *generic_domain;
} iova_domain_t;

typedef struct dma_buffer {
    void *cpu_addr;
    uint64_t phys_addr;
    uint64_t iova;
    size_t size;
    uint32_t flags;

    // Pin Accounting
    uint64_t pin_count;
    uint64_t owner_as_id; // Address space ID owning the pinned memory

    // Binding
    iova_domain_t *domain;
    dma_direction_t active_dir;
    bool mapped_to_device;
    bool owned_by_device;

    struct dma_buffer *next;
} dma_buffer_t;

// Pin Budgets
#define DMA_MAX_PIN_BYTES_PER_PROCESS (64ULL * 1024ULL * 1024ULL) // 64MB default quota

// Accounting global struct (can be embedded in `vm_address_space_t` eventually)
int dma_account_pin(uint64_t as_id, size_t bytes);
void dma_account_unpin(uint64_t as_id, size_t bytes);

int dma_buffer_alloc(size_t size, uint32_t flags, dma_buffer_t **out);
int dma_buffer_pin(dma_buffer_t *buffer, uint64_t as_id);
int dma_buffer_unpin(dma_buffer_t *buffer);
int dma_buffer_free(dma_buffer_t *buffer);

// IOVA Domains
int iova_domain_create(uint64_t base, uint64_t limit, iova_domain_t **out_domain);
int iova_domain_destroy(iova_domain_t *domain);
int iova_alloc(iova_domain_t *domain, size_t size, uint64_t *out_iova);
void iova_free(iova_domain_t *domain, uint64_t iova, size_t size);
int dma_buffer_bind_domain(dma_buffer_t *buffer, iova_domain_t *domain);
int dma_buffer_map_device(uint64_t device_id, dma_buffer_t *buffer, dma_direction_t dir);
int dma_buffer_unmap_device(uint64_t device_id, dma_buffer_t *buffer, dma_direction_t dir);
void dma_sync_for_device(dma_buffer_t *buffer, dma_direction_t dir);
void dma_sync_for_cpu(dma_buffer_t *buffer, dma_direction_t dir);


struct dma_caps {
    uint64_t dma_mask;
    uint64_t max_segment_size;
    size_t   alignment;
    bool     coherent;
    bool     require_isolation;
    bool     allow_bypass;
    bool     allow_identity;
    bool     require_contiguous;
};

typedef struct dma_mapping {
    iommu_device_t *dev;
    void *cpu_addr;
    uint64_t dma_addr;
    uint64_t phys_addr;
    size_t len;
    dma_direction_t dir;
    dma_translation_mode_t mode;
    bool coherent;
    bool bounced;
    struct dma_mapping *next;
} dma_mapping_t;

int dma_map_buffer(iommu_device_t *dev, void *cpu_addr, uint64_t phys_addr, size_t len,
                   dma_direction_t dir, dma_mapping_t **out_mapping);

int dma_unmap_buffer(dma_mapping_t *mapping);

void *dma_alloc_coherent(iommu_device_t *dev, size_t len, uint64_t *dma_addr_out, dma_mapping_t **out_mapping);
void dma_free_coherent(dma_mapping_t *mapping);

#ifdef __cplusplus
}
#endif

#endif // BHARAT_MM_DMA_H
