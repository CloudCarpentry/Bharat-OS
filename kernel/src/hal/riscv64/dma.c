#include "../../../include/hal/hal_dma.h"
#include "../../../include/mm.h"
#include "../../../include/numa.h"

// External allocators/deallocators
extern void *kmalloc(size_t size);
extern void kfree(void *ptr);

#define RISCV_CACHE_LINE_BYTES 64

// TODO: If Zicbom is available, use `cbo.clean` and `cbo.inval`
// We will stub cache operations out if the platform isn't Zicbom capable yet.
// For correct operation, we shouldn't lie about correctness.
static inline void riscv_dcache_clean_range(uintptr_t start, size_t size) {
    // If Zicbom is not guaranteed, explicitly do nothing or fail.
    // In a real port, we might check an ISA extension bit here and issue `cbo.clean`.
    (void)start;
    (void)size;
}

static inline void riscv_dcache_invalidate_range(uintptr_t start, size_t size) {
    (void)start;
    (void)size;
}

static inline void riscv_dcache_clean_invalidate_range(uintptr_t start, size_t size) {
    (void)start;
    (void)size;
}

static int riscv64_dma_alloc(size_t size, uint32_t flags, hal_dma_buffer_t *out_buf) {
    if (!out_buf || size == 0) return -1;

    uint32_t numa_node = (flags & 2) ? NUMA_NODE_LOCAL : NUMA_NODE_ANY; // 2 -> DMA_ALLOC_DMA32
    int order = 0;
    while ((1ULL << (order + 12)) < size) order++;

    phys_addr_t pa = mm_alloc_pages_order(order, numa_node, flags);
    if (!pa) return -3;

    out_buf->phys_addr = pa;
    out_buf->cpu_addr = (void *)(uintptr_t)pa; // Assume 1:1 kernel phys map for now
    out_buf->size = size;
    out_buf->flags = flags;
    out_buf->dma_addr = pa; // Assume dma = phys
    out_buf->dir = HAL_DMA_NONE;
    out_buf->backend_priv = NULL;

    if (flags & 4) { // 4 -> DMA_ALLOC_ZERO
        uint8_t *ptr = (uint8_t *)out_buf->cpu_addr;
        for (size_t i = 0; i < size; i++) ptr[i] = 0;
    }

    return 0;
}

static void riscv64_dma_free(hal_dma_buffer_t *buf) {
    if (!buf || !buf->phys_addr) return;

    int order = 0;
    while ((1ULL << (order + 12)) < buf->size) order++;
    size_t alloc_size = (1ULL << (order + 12));

    for (size_t i = 0; i < alloc_size; i += PAGE_SIZE) {
        mm_free_page(buf->phys_addr + i);
    }
}

static int riscv64_dma_map(hal_dma_buffer_t *buf) {
    if (!buf) return -1;
    buf->dma_addr = buf->phys_addr;

    if (buf->dir == HAL_DMA_TO_DEVICE || buf->dir == HAL_DMA_BIDIRECTIONAL) {
        riscv_dcache_clean_range((uintptr_t)buf->cpu_addr, buf->size);
    }

    return 0;
}

static void riscv64_dma_unmap(hal_dma_buffer_t *buf) {
    if (!buf) return;

    if (buf->dir == HAL_DMA_FROM_DEVICE || buf->dir == HAL_DMA_BIDIRECTIONAL) {
        riscv_dcache_invalidate_range((uintptr_t)buf->cpu_addr, buf->size);
    }
}

static void riscv64_dma_sync_for_device(hal_dma_buffer_t *buf) {
    if (!buf || !buf->cpu_addr || buf->size == 0) return;

    if (buf->dir == HAL_DMA_TO_DEVICE) {
        riscv_dcache_clean_range((uintptr_t)buf->cpu_addr, buf->size);
    } else if (buf->dir == HAL_DMA_BIDIRECTIONAL) {
        riscv_dcache_clean_invalidate_range((uintptr_t)buf->cpu_addr, buf->size);
    }
}

static void riscv64_dma_sync_for_cpu(hal_dma_buffer_t *buf) {
    if (!buf || !buf->cpu_addr || buf->size == 0) return;

    if (buf->dir == HAL_DMA_FROM_DEVICE || buf->dir == HAL_DMA_BIDIRECTIONAL) {
        riscv_dcache_invalidate_range((uintptr_t)buf->cpu_addr, buf->size);
    }
}

static int riscv64_dma_is_coherent(void) {
    // RV systems vary wildly. Assume false for safety unless known coherent.
    return 0;
}

static size_t riscv64_dma_cache_line_size(void) {
    return RISCV_CACHE_LINE_BYTES;
}

static size_t riscv64_dma_get_alignment(void) {
    return RISCV_CACHE_LINE_BYTES;
}

static int riscv64_dma_needs_sync(void) {
    return 1;
}

static const hal_dma_ops_t riscv64_dma_ops = {
    .alloc = riscv64_dma_alloc,
    .free = riscv64_dma_free,
    .map = riscv64_dma_map,
    .unmap = riscv64_dma_unmap,
    .sync_for_device = riscv64_dma_sync_for_device,
    .sync_for_cpu = riscv64_dma_sync_for_cpu,
    .is_coherent = riscv64_dma_is_coherent,
    .cache_line_size = riscv64_dma_cache_line_size,
    .get_alignment = riscv64_dma_get_alignment,
    .needs_sync = riscv64_dma_needs_sync,
};

void hal_dma_backend_init(void);
void hal_dma_backend_init(void) {
    hal_dma_register_ops(&riscv64_dma_ops);
}
