#ifndef BHARAT_HAL_DMA_H
#define BHARAT_HAL_DMA_H

#include <stddef.h>
#include <stdint.h>

typedef enum {
    HAL_DMA_TO_DEVICE = 0,
    HAL_DMA_FROM_DEVICE = 1,
    HAL_DMA_BIDIRECTIONAL = 2,
} hal_dma_direction_t;

typedef struct hal_dma_ops {
    void (*sync_for_device)(void *cpu_addr, uint64_t phys_addr, size_t size, hal_dma_direction_t dir);
    void (*sync_for_cpu)(void *cpu_addr, uint64_t phys_addr, size_t size, hal_dma_direction_t dir);
    int  (*is_coherent)(void);
} hal_dma_ops_t;

void hal_dma_register_ops(const hal_dma_ops_t *ops);

void hal_dma_sync_for_device(void *cpu_addr, uint64_t phys_addr, size_t size, hal_dma_direction_t dir);
void hal_dma_sync_for_cpu(void *cpu_addr, uint64_t phys_addr, size_t size, hal_dma_direction_t dir);
int hal_dma_is_coherent(void);

#endif // BHARAT_HAL_DMA_H
