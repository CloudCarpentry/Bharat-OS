#include "../../kernel/include/hal/hal_dma.h"
#include "../../kernel/include/mm.h"
#include "../../kernel/include/numa.h"

// External allocators/deallocators
extern void *kmalloc(size_t size);
extern void kfree(void *ptr);

#define L1_CACHE_BYTES 64 // Usually 64 bytes on ARMv8

static inline void dcache_clean_range(uintptr_t start, size_t size) {
    uintptr_t line_size = L1_CACHE_BYTES;
    uintptr_t end = start + size;
    start &= ~(line_size - 1);
    while (start < end) {
        __asm__ __volatile__("dc cvac, %0" : : "r"(start) : "memory");
        start += line_size;
    }
    __asm__ __volatile__("dsb sy" ::: "memory");
}

static inline void dcache_invalidate_range(uintptr_t start, size_t size) {
    uintptr_t line_size = L1_CACHE_BYTES;
    uintptr_t end = start + size;
    start &= ~(line_size - 1);
    while (start < end) {
        __asm__ __volatile__("dc ivac, %0" : : "r"(start) : "memory");
        start += line_size;
    }
    __asm__ __volatile__("dsb sy" ::: "memory");
}

static inline void dcache_clean_invalidate_range(uintptr_t start, size_t size) {
    uintptr_t line_size = L1_CACHE_BYTES;
    uintptr_t end = start + size;
    start &= ~(line_size - 1);
    while (start < end) {
        __asm__ __volatile__("dc civac, %0" : : "r"(start) : "memory");
        start += line_size;
    }
    __asm__ __volatile__("dsb sy" ::: "memory");
}

static int arm64_dma_alloc(size_t size, uint32_t flags, hal_dma_buffer_t *out_buf) {
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

static void arm64_dma_free(hal_dma_buffer_t *buf) {
    if (!buf || !buf->phys_addr) return;

    int order = 0;
    while ((1ULL << (order + 12)) < buf->size) order++;
    size_t alloc_size = (1ULL << (order + 12));

    for (size_t i = 0; i < alloc_size; i += PAGE_SIZE) {
        mm_free_page(buf->phys_addr + i);
    }
}

static int arm64_dma_map(hal_dma_buffer_t *buf) {
    if (!buf) return -1;
    buf->dma_addr = buf->phys_addr;

    if (buf->dir == HAL_DMA_TO_DEVICE || buf->dir == HAL_DMA_BIDIRECTIONAL) {
        dcache_clean_range((uintptr_t)buf->cpu_addr, buf->size);
    }

    return 0;
}

static void arm64_dma_unmap(hal_dma_buffer_t *buf) {
    if (!buf) return;

    if (buf->dir == HAL_DMA_FROM_DEVICE || buf->dir == HAL_DMA_BIDIRECTIONAL) {
        dcache_invalidate_range((uintptr_t)buf->cpu_addr, buf->size);
    }
}

static void arm64_dma_sync_for_device(hal_dma_buffer_t *buf) {
    if (!buf || !buf->cpu_addr || buf->size == 0) return;

    if (buf->dir == HAL_DMA_TO_DEVICE) {
        dcache_clean_range((uintptr_t)buf->cpu_addr, buf->size);
    } else if (buf->dir == HAL_DMA_BIDIRECTIONAL) {
        dcache_clean_invalidate_range((uintptr_t)buf->cpu_addr, buf->size);
    }
}

static void arm64_dma_sync_for_cpu(hal_dma_buffer_t *buf) {
    if (!buf || !buf->cpu_addr || buf->size == 0) return;

    if (buf->dir == HAL_DMA_FROM_DEVICE || buf->dir == HAL_DMA_BIDIRECTIONAL) {
        dcache_invalidate_range((uintptr_t)buf->cpu_addr, buf->size);
    }
}

static int arm64_dma_is_coherent(void) {
    return 0; // Not always coherent on ARM64
}

static size_t arm64_dma_cache_line_size(void) {
    return L1_CACHE_BYTES;
}

static size_t arm64_dma_get_alignment(void) {
    return L1_CACHE_BYTES;
}

static int arm64_dma_needs_sync(void) {
    return 1;
}

static const hal_dma_ops_t arm64_dma_ops = {
    .alloc = arm64_dma_alloc,
    .free = arm64_dma_free,
    .map = arm64_dma_map,
    .unmap = arm64_dma_unmap,
    .sync_for_device = arm64_dma_sync_for_device,
    .sync_for_cpu = arm64_dma_sync_for_cpu,
    .is_coherent = arm64_dma_is_coherent,
    .cache_line_size = arm64_dma_cache_line_size,
    .get_alignment = arm64_dma_get_alignment,
    .needs_sync = arm64_dma_needs_sync,
};

void hal_dma_backend_init(void);
void hal_dma_backend_init(void) {
    hal_dma_register_ops(&arm64_dma_ops);
}
