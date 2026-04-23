#include "hal/hal_iommu.h"
#include "hal/hal_dma.h"
#include "mm/dma.h"
#include "mm.h"
#include "mm/physmap.h"
#include "numa.h"
#include "spinlock.h"

// External allocators
extern void *kmalloc(size_t size);
extern void kfree(void *ptr);

static hal_dma_direction_t dma_to_hal_direction(dma_direction_t dir) {
    switch (dir) {
        case DMA_MAP_FROM_DEVICE:
            return HAL_DMA_FROM_DEVICE;
        case DMA_MAP_BIDIRECTIONAL:
            return HAL_DMA_BIDIRECTIONAL;
        case DMA_MAP_TO_DEVICE:
        default:
            return HAL_DMA_TO_DEVICE;
    }
}

// Basic global accounting (in reality, belongs per process/aspace structure)
static spinlock_t global_pin_lock;
static uint64_t global_pinned_bytes = 0; // Simple accounting for demo
static int global_pin_lock_ready = 0;

static inline void dma_accounting_init_once(void) {
    if (!global_pin_lock_ready) {
        spin_lock_init(&global_pin_lock);
        global_pin_lock_ready = 1;
    }
}

static inline int dma_direction_valid(dma_direction_t dir) {
    return (dir == DMA_MAP_TO_DEVICE) ||
           (dir == DMA_MAP_FROM_DEVICE) ||
           (dir == DMA_MAP_BIDIRECTIONAL);
}

int dma_account_pin(uint64_t as_id, size_t bytes) {
    (void)as_id; // Per-process check goes here
    dma_accounting_init_once();

    spin_lock(&global_pin_lock);
    if (bytes > DMA_MAX_PIN_BYTES_PER_PROCESS ||
        global_pinned_bytes > (DMA_MAX_PIN_BYTES_PER_PROCESS - bytes)) {
        spin_unlock(&global_pin_lock);
        return -1; // Exceeded budget
    }
    global_pinned_bytes += bytes;
    spin_unlock(&global_pin_lock);
    return 0;
}

void dma_account_unpin(uint64_t as_id, size_t bytes) {
    (void)as_id;
    dma_accounting_init_once();
    spin_lock(&global_pin_lock);
    if (global_pinned_bytes >= bytes) {
        global_pinned_bytes -= bytes;
    }
    spin_unlock(&global_pin_lock);
}

// Basic IOVA allocator using a simple bump allocator for demo purposes
// A production system uses a red-black tree (e.g. Linux IOVA rbtree) or bitmap
typedef struct {
    uint64_t next_free;
} iova_bump_allocator_t;

int iova_domain_create(uint64_t base, uint64_t limit, iova_domain_t **out_domain) {
    if (!out_domain || limit <= base) return -1;

    iova_domain_t *dom = (iova_domain_t *)kmalloc(sizeof(iova_domain_t));
    if (!dom) return -2;

    dom->id = 1; // Generic ID
    dom->base = base;
    dom->limit = limit;
    spin_lock_init(&dom->lock);

    iova_bump_allocator_t *alloc = (iova_bump_allocator_t *)kmalloc(sizeof(iova_bump_allocator_t));
    if (!alloc) {
        kfree(dom);
        return -2;
    }
    alloc->next_free = base;
    dom->allocator_data = alloc;

    dom->iommu = NULL;
    dom->iommu_hw_state = NULL;

    *out_domain = dom;
    return 0;
}

int iova_domain_destroy(iova_domain_t *domain) {
    if (!domain) return -1;

    if (domain->allocator_data) {
        kfree(domain->allocator_data);
    }
    kfree(domain);
    return 0;
}

int iova_alloc(iova_domain_t *domain, size_t size, uint64_t *out_iova) {
    if (!domain || !out_iova || size == 0) return -1;

    size = (size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1); // Align

    spin_lock(&domain->lock);
    iova_bump_allocator_t *alloc = (iova_bump_allocator_t *)domain->allocator_data;

    if (alloc->next_free + size > domain->limit) {
        spin_unlock(&domain->lock);
        return -3; // OOM in IOVA space
    }

    *out_iova = alloc->next_free;
    alloc->next_free += size;

    spin_unlock(&domain->lock);
    return 0;
}

void iova_free(iova_domain_t *domain, uint64_t iova, size_t size) {
    if (!domain) return;
    // Bump allocator cannot easily free unless it's the last allocation.
    // Real implementation removes the node from the rbtree.
    (void)iova;
    (void)size;
}

// DMA Buffer Objects
int dma_buffer_alloc(size_t size, uint32_t flags, dma_buffer_t **out) {
    if (!out || size == 0) return -1;

    dma_buffer_t *buf = (dma_buffer_t *)kmalloc(sizeof(dma_buffer_t));
    if (!buf) return -2;

    // Allocate contiguous physical pages for simple DMA
    // (If using an IOMMU, pages don't need to be contiguous, but we keep it simple here)
    uint32_t numa_node = (flags & DMA_ALLOC_DMA32) ? NUMA_NODE_LOCAL : NUMA_NODE_ANY;
    int order = 0; // Find correct order for size
    while ((1ULL << (order + 12)) < size) order++;

    phys_addr_t pa = mm_alloc_pages_order(order, numa_node, flags);
    if (!pa) {
        kfree(buf);
        return -3;
    }

    buf->phys_addr = pa;
    buf->size = size;
    buf->flags = flags;
    buf->cpu_addr = physmap_phys_to_virt(pa);
    buf->iova = 0;
    buf->pin_count = 0;
    buf->owner_as_id = 0;
    buf->domain = NULL;
    buf->active_dir = DMA_MAP_TO_DEVICE;
    buf->mapped_to_device = false;
    buf->owned_by_device = false;
    buf->next = NULL;

    // Zero memory if requested
    if (flags & DMA_ALLOC_ZERO) {
        uint8_t *ptr = (uint8_t *)buf->cpu_addr;
        for (size_t i = 0; i < size; i++) ptr[i] = 0;
    }

    *out = buf;
    return 0;
}

int dma_buffer_free(dma_buffer_t *buffer) {
    if (!buffer) return -1;
    if (buffer->mapped_to_device) return -2;

    if (buffer->pin_count > 0) {
        // Force unpin
        dma_account_unpin(buffer->owner_as_id, buffer->size);
    }

    if (buffer->domain && buffer->iova != 0) {
        iova_free(buffer->domain, buffer->iova, buffer->size);
    }

    // Free the physical pages
    int order = 0;
    while ((1ULL << (order + 12)) < buffer->size) order++;
    // Actual mm_free_pages_order required here, falling back to basic if unavialable
    // For a multi-page allocation, we must free each page individually if we lack an order-based free
    for (size_t i = 0; i < buffer->size; i += PAGE_SIZE) {
        mm_free_page(buffer->phys_addr + i);
    }

    kfree(buffer);
    return 0;
}

int dma_buffer_pin(dma_buffer_t *buffer, uint64_t as_id) {
    if (!buffer) return -1;

    if (buffer->pin_count == 0) {
        int ret = dma_account_pin(as_id, buffer->size);
        if (ret != 0) return ret; // Exceeded pin budget
        buffer->owner_as_id = as_id;
    }

    buffer->pin_count++;
    return 0;
}

int dma_buffer_unpin(dma_buffer_t *buffer) {
    if (!buffer || buffer->pin_count == 0) return -1;

    buffer->pin_count--;

    if (buffer->pin_count == 0) {
        dma_account_unpin(buffer->owner_as_id, buffer->size);
        buffer->owner_as_id = 0;
    }
    return 0;
}

int dma_buffer_bind_domain(dma_buffer_t *buffer, iova_domain_t *domain) {
    if (!buffer || !domain) return -1;

    if (buffer->iova != 0) return -2; // Already bound
    if (buffer->mapped_to_device) return -3;

    uint64_t iova = 0;
    int ret = iova_alloc(domain, buffer->size, &iova);
    if (ret != 0) return ret;

    buffer->domain = domain;
    buffer->iova = iova;

    // Map in IOMMU if backend is present
    if (domain->iommu) {
         if (domain->iommu_hw_state) {
            hal_iommu_domain_t *hw_dom = (hal_iommu_domain_t*)domain->iommu_hw_state;
            int map_ret = hal_iommu_map(hw_dom, iova, buffer->phys_addr, buffer->size, buffer->flags);
            if (map_ret != 0) {
                iova_free(domain, iova, buffer->size);
                buffer->domain = NULL;
                buffer->iova = 0;
                return map_ret;
            }
        }
    }

    return 0;
}

void dma_sync_for_device(dma_buffer_t *buffer, dma_direction_t dir) {
    if (!buffer || !buffer->cpu_addr || buffer->size == 0) {
        return;
    }
    if (!dma_direction_valid(dir)) {
        return;
    }
    if (buffer->owned_by_device && buffer->active_dir == dir) {
        return;
    }
    if (hal_dma_needs_sync()) {
        hal_dma_buffer_t hbuf;
        hbuf.cpu_addr = buffer->cpu_addr;
        hbuf.phys_addr = buffer->phys_addr;
        hbuf.dma_addr = buffer->iova;
        hbuf.size = buffer->size;
        hbuf.dir = dma_to_hal_direction(dir);
        hbuf.flags = buffer->flags;
        hbuf.backend_priv = NULL;
        hal_dma_sync_for_device(&hbuf);
    }
}

void dma_sync_for_cpu(dma_buffer_t *buffer, dma_direction_t dir) {
    if (!buffer || !buffer->cpu_addr || buffer->size == 0) {
        return;
    }
    if (!dma_direction_valid(dir)) {
        return;
    }
    if (!buffer->owned_by_device) {
        return;
    }
    if (hal_dma_needs_sync()) {
        hal_dma_buffer_t hbuf;
        hbuf.cpu_addr = buffer->cpu_addr;
        hbuf.phys_addr = buffer->phys_addr;
        hbuf.dma_addr = buffer->iova;
        hbuf.size = buffer->size;
        hbuf.dir = dma_to_hal_direction(dir);
        hbuf.flags = buffer->flags;
        hbuf.backend_priv = NULL;
        hal_dma_sync_for_cpu(&hbuf);
    }
}

int dma_buffer_map_device(uint64_t device_id, dma_buffer_t *buffer, dma_direction_t dir) {
    (void)device_id;
    if (!buffer) return -1;
    if (buffer->pin_count == 0) return -2;
    if (!dma_direction_valid(dir)) return -3;
    if (buffer->mapped_to_device) return -4;

    hal_dma_buffer_t hbuf;
    hbuf.cpu_addr = buffer->cpu_addr;
    hbuf.phys_addr = buffer->phys_addr;
    hbuf.dma_addr = (buffer->iova != 0) ? buffer->iova : buffer->phys_addr;
    hbuf.size = buffer->size;
    hbuf.dir = dma_to_hal_direction(dir);
    hbuf.flags = buffer->flags;
    hbuf.backend_priv = NULL;

    if (buffer->domain && buffer->iova != 0) {
        hal_iommu_domain_t *hw_dom = (hal_iommu_domain_t *)buffer->domain->iommu_hw_state;
        if (!hw_dom) return -5;
        int map_ret = hal_iommu_map(hw_dom, buffer->iova, buffer->phys_addr, buffer->size, (uint64_t)dir);
        if (map_ret != 0) return map_ret;
    }

    int hal_ret = hal_dma_map_buffer(&hbuf);
    if (hal_ret != 0) {
        if (buffer->domain && buffer->iova != 0 && buffer->domain->iommu_hw_state) {
            hal_iommu_domain_t *hw_dom = (hal_iommu_domain_t *)buffer->domain->iommu_hw_state;
            (void)hal_iommu_unmap(hw_dom, buffer->iova, buffer->size);
        }
        return hal_ret;
    }
    dma_sync_for_device(buffer, dir);
    buffer->mapped_to_device = true;
    buffer->owned_by_device = true;
    buffer->active_dir = dir;
    buffer->iova = hbuf.dma_addr;
    return 0;
}

int dma_buffer_unmap_device(uint64_t device_id, dma_buffer_t *buffer, dma_direction_t dir) {
    (void)device_id;
    if (!buffer) return -1;
    if (!buffer->mapped_to_device) return -2;
    if (dir != buffer->active_dir) return -3;

    dma_sync_for_cpu(buffer, dir);

    hal_dma_buffer_t hbuf;
    hbuf.cpu_addr = buffer->cpu_addr;
    hbuf.phys_addr = buffer->phys_addr;
    hbuf.dma_addr = buffer->iova;
    hbuf.size = buffer->size;
    hbuf.dir = dma_to_hal_direction(dir);
    hbuf.flags = buffer->flags;
    hbuf.backend_priv = NULL;
    hal_dma_unmap_buffer(&hbuf);

    if (buffer->domain && buffer->iova != 0) {
        hal_iommu_domain_t *hw_dom = (hal_iommu_domain_t *)buffer->domain->iommu_hw_state;
        if (!hw_dom) return -4;
        int unmap_ret = hal_iommu_unmap(hw_dom, buffer->iova, buffer->size);
        if (unmap_ret != 0) return unmap_ret;
    }

    buffer->mapped_to_device = false;
    buffer->owned_by_device = false;
    return 0;
}
