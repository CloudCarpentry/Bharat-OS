#ifndef BHARAT_HAL_DMA_H
#define BHARAT_HAL_DMA_H

#include <stddef.h>
#include <stdint.h>

typedef enum {
    HAL_DMA_NONE = -1,
    HAL_DMA_TO_DEVICE = 0,
    HAL_DMA_FROM_DEVICE = 1,
    HAL_DMA_BIDIRECTIONAL = 2,
} hal_dma_direction_t;

typedef struct hal_dma_buffer {
    void *cpu_addr;
    uint64_t phys_addr;
    uint64_t dma_addr;
    size_t size;
    hal_dma_direction_t dir;
    uint32_t flags;
    void *backend_priv;
} hal_dma_buffer_t;

typedef struct hal_dma_ops {
    int (*alloc)(size_t size, uint32_t flags, hal_dma_buffer_t *out_buf);
    void (*free)(hal_dma_buffer_t *buf);
    int (*map)(hal_dma_buffer_t *buf);
    void (*unmap)(hal_dma_buffer_t *buf);
    void (*sync_for_device)(hal_dma_buffer_t *buf);
    void (*sync_for_cpu)(hal_dma_buffer_t *buf);
    int (*is_coherent)(void);
    size_t (*cache_line_size)(void);
    size_t (*get_alignment)(void);
    int (*needs_sync)(void);
} hal_dma_ops_t;

void hal_dma_register_ops(const hal_dma_ops_t *ops);

int hal_dma_alloc(size_t size, uint32_t flags, hal_dma_buffer_t *out_buf);
void hal_dma_free(hal_dma_buffer_t *buf);
int hal_dma_map_buffer(hal_dma_buffer_t *buf);
void hal_dma_unmap_buffer(hal_dma_buffer_t *buf);
void hal_dma_sync_for_device(hal_dma_buffer_t *buf);
void hal_dma_sync_for_cpu(hal_dma_buffer_t *buf);

int hal_dma_is_coherent(void);
size_t hal_dma_cache_line_size(void);
size_t hal_dma_get_alignment(void);
int hal_dma_needs_sync(void);

#endif // BHARAT_HAL_DMA_H
