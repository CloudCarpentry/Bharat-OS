#ifndef BHARAT_MM_DMA_H
#define BHARAT_MM_DMA_H

#include <stddef.h>
#include <stdint.h>

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

typedef struct {
    void *cpu_addr;
    uint64_t phys_addr;
    uint64_t iova;
    size_t size;
    uint32_t flags;
} dma_buffer_t;

int dma_buffer_alloc(size_t size, uint32_t flags, dma_buffer_t *out);
int dma_buffer_free(dma_buffer_t *buffer);
int dma_buffer_map_device(uint64_t device_id, dma_buffer_t *buffer, dma_direction_t dir);
int dma_buffer_unmap_device(uint64_t device_id, dma_buffer_t *buffer, dma_direction_t dir);

#ifdef __cplusplus
}
#endif

#endif // BHARAT_MM_DMA_H
