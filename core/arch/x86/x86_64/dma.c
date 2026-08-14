#include "../../kernel/include/hal/hal_dma.h"
#include "hal/hal_iommu.h"
#include "../../kernel/include/numa.h"
#include "../../kernel/include/mm.h"

// External allocators/deallocators
extern void *kmalloc(size_t size);
extern void kfree(void *ptr);

static int x86_64_dma_alloc(size_t size, uint32_t flags, hal_dma_buffer_t *out_buf) {
    if (!out_buf || size == 0) return -1;

    uint32_t numa_node = (flags & 2) ? NUMA_NODE_LOCAL : NUMA_NODE_ANY; // 2 -> DMA_ALLOC_DMA32
    int order = 0;
    while ((1ULL << (order + 12)) < size) order++;

    phys_addr_t pa = mm_alloc_pages_order(order, numa_node, flags);
    if (!pa) return -3;

    out_buf->phys_addr = pa;
    out_buf->cpu_addr = (void *)(uintptr_t)pa; // Kernel 1:1 mapping
    out_buf->size = size;
    out_buf->flags = flags;
    out_buf->dma_addr = 0;
    out_buf->dir = HAL_DMA_NONE;
    out_buf->backend_priv = NULL;

    if (flags & 4) { // 4 -> DMA_ALLOC_ZERO
        uint8_t *ptr = (uint8_t *)out_buf->cpu_addr;
        for (size_t i = 0; i < size; i++) ptr[i] = 0;
    }

    return 0;
}

static void x86_64_dma_free(hal_dma_buffer_t *buf) {
    if (!buf || !buf->phys_addr) return;

    int order = 0;
    while ((1ULL << (order + 12)) < buf->size) order++;
    size_t alloc_size = (1ULL << (order + 12));

    for (size_t i = 0; i < alloc_size; i += PAGE_SIZE) {
        mm_free_page(buf->phys_addr + i);
    }
}

static int x86_64_dma_map(hal_dma_buffer_t *buf) {
    if (!buf) return -1;
    // On x86_64 without IOMMU mapped explicitly, DMA address is physical address
    buf->dma_addr = buf->phys_addr;
    return 0;
}

static void x86_64_dma_unmap(hal_dma_buffer_t *buf) {
    if (!buf) return;
    // No-op for direct mapping
}

static void x86_64_dma_sync_for_device(hal_dma_buffer_t *buf) {
    // x86_64 cache is coherent for DMA
    (void)buf;
    // Explicit compiler barrier just in case
    __asm__ __volatile__("" ::: "memory");
}

static void x86_64_dma_sync_for_cpu(hal_dma_buffer_t *buf) {
    // x86_64 cache is coherent for DMA
    (void)buf;
    // Explicit compiler barrier just in case
    __asm__ __volatile__("" ::: "memory");
}

static int x86_64_dma_is_coherent(void) {
    return 1; // x86_64 is strongly coherent
}

static size_t x86_64_dma_cache_line_size(void) {
    return 64; // Common x86_64 cache line size
}

static size_t x86_64_dma_get_alignment(void) {
    return 64;
}

static int x86_64_dma_needs_sync(void) {
    return 0; // Coherent-by-default
}

static const hal_dma_ops_t x86_64_dma_ops = {
    .alloc = x86_64_dma_alloc,
    .free = x86_64_dma_free,
    .map = x86_64_dma_map,
    .unmap = x86_64_dma_unmap,
    .sync_for_device = x86_64_dma_sync_for_device,
    .sync_for_cpu = x86_64_dma_sync_for_cpu,
    .is_coherent = x86_64_dma_is_coherent,
    .cache_line_size = x86_64_dma_cache_line_size,
    .get_alignment = x86_64_dma_get_alignment,
    .needs_sync = x86_64_dma_needs_sync,
};

void hal_dma_backend_init(void);
void hal_dma_backend_init(void) {
    hal_dma_register_ops(&x86_64_dma_ops);
}
