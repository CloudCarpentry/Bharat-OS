#include "../../include/hal/hal_dma.h"

static const hal_dma_ops_t *g_hal_dma_ops;

void hal_dma_register_ops(const hal_dma_ops_t *ops) {
    g_hal_dma_ops = ops;
}

int hal_dma_alloc(size_t size, uint32_t flags, hal_dma_buffer_t *out_buf) {
    if (!out_buf || size == 0) return -1;
    if (g_hal_dma_ops && g_hal_dma_ops->alloc) {
        return g_hal_dma_ops->alloc(size, flags, out_buf);
    }
    return -1; // Fallback: no default allocator
}

void hal_dma_free(hal_dma_buffer_t *buf) {
    if (!buf) return;
    if (g_hal_dma_ops && g_hal_dma_ops->free) {
        g_hal_dma_ops->free(buf);
    }
}

int hal_dma_map_buffer(hal_dma_buffer_t *buf) {
    if (!buf) return -1;
    if (buf->size == 0 || buf->dir == HAL_DMA_NONE) return -2; // Validation
    if (g_hal_dma_ops && g_hal_dma_ops->map) {
        return g_hal_dma_ops->map(buf);
    }
    // Default fallback: identity mapping if no map op
    buf->dma_addr = buf->phys_addr;
    return 0;
}

void hal_dma_unmap_buffer(hal_dma_buffer_t *buf) {
    if (!buf) return;
    if (g_hal_dma_ops && g_hal_dma_ops->unmap) {
        g_hal_dma_ops->unmap(buf);
    }
}

void hal_dma_sync_for_device(hal_dma_buffer_t *buf) {
    if (!buf) return;
    if (buf->size == 0) return;
    if (buf->dir == HAL_DMA_NONE || buf->dir == HAL_DMA_FROM_DEVICE) return; // Validation

    if (g_hal_dma_ops && g_hal_dma_ops->sync_for_device) {
        g_hal_dma_ops->sync_for_device(buf);
    }
}

void hal_dma_sync_for_cpu(hal_dma_buffer_t *buf) {
    if (!buf) return;
    if (buf->size == 0) return;
    if (buf->dir == HAL_DMA_NONE || buf->dir == HAL_DMA_TO_DEVICE) return; // Validation

    if (g_hal_dma_ops && g_hal_dma_ops->sync_for_cpu) {
        g_hal_dma_ops->sync_for_cpu(buf);
    }
}

// TODO: Needs refactor: #include directive placed mid-file for dependency/order compatibility.
#include "arch/arch_caps.h"

int hal_dma_is_coherent(void) {
    if (arch_has_cap(ARCH_CAP_DMA_COHERENT)) {
        return 1;
    }

    if (g_hal_dma_ops && g_hal_dma_ops->is_coherent) {
        return g_hal_dma_ops->is_coherent();
    }
    return 1; // Default coherent
}

size_t hal_dma_cache_line_size(void) {
    if (g_hal_dma_ops && g_hal_dma_ops->cache_line_size) {
        return g_hal_dma_ops->cache_line_size();
    }
    return 64; // Safe default
}

size_t hal_dma_get_alignment(void) {
    if (g_hal_dma_ops && g_hal_dma_ops->get_alignment) {
        return g_hal_dma_ops->get_alignment();
    }
    return 64; // Safe default
}

int hal_dma_needs_sync(void) {
    if (g_hal_dma_ops && g_hal_dma_ops->needs_sync) {
        return g_hal_dma_ops->needs_sync();
    }
    return !hal_dma_is_coherent();
}
