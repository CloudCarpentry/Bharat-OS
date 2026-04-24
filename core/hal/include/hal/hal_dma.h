#ifndef BHARAT_HAL_DMA_H
#define BHARAT_HAL_DMA_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    DMA_DIRECTION_TO_DEVICE,
    DMA_DIRECTION_FROM_DEVICE,
    DMA_DIRECTION_BIDIRECTIONAL
} hal_dma_direction_t;

typedef enum {
    DMA_SYNC_FOR_CPU,
    DMA_SYNC_FOR_DEVICE
} hal_dma_sync_target_t;

/**
 * Maps a CPU-visible physical address to a device-visible address (IOVA or Phys).
 * Performs necessary cache maintenance (clean) for non-coherent devices.
 *
 * @param phys_addr The CPU physical address of the buffer.
 * @param size The size of the buffer.
 * @param dir The direction of the DMA transfer.
 * @param out_dma_addr Pointer to store the resulting device address.
 * @return 0 on success, negative error code on failure.
 */
int hal_dma_map(uint64_t phys_addr, size_t size, hal_dma_direction_t dir, uint64_t *out_dma_addr);

/**
 * Unmaps a previously mapped DMA address.
 * Performs necessary cache maintenance (invalidate) for non-coherent devices.
 *
 * @param dma_addr The device address to unmap.
 * @param size The size of the buffer.
 * @param dir The direction of the DMA transfer.
 * @return 0 on success, negative error code on failure.
 */
int hal_dma_unmap(uint64_t dma_addr, size_t size, hal_dma_direction_t dir);

/**
 * Synchronizes a streaming DMA buffer between the CPU and the device.
 *
 * @param dma_addr The device address.
 * @param size The size of the synchronized region.
 * @param target Whether the CPU or the device is taking ownership.
 * @return 0 on success, negative error code on failure.
 */
int hal_dma_sync(uint64_t dma_addr, size_t size, hal_dma_sync_target_t target);

#ifdef __cplusplus
}
#endif

#endif /* BHARAT_HAL_DMA_H */
