#include "../../include/hal/hal_dma.h"

static const hal_dma_ops_t *g_hal_dma_ops;

void hal_dma_register_ops(const hal_dma_ops_t *ops) {
    g_hal_dma_ops = ops;
}

void hal_dma_sync_for_device(void *cpu_addr, uint64_t phys_addr, size_t size, hal_dma_direction_t dir) {
    if (g_hal_dma_ops && g_hal_dma_ops->sync_for_device) {
        g_hal_dma_ops->sync_for_device(cpu_addr, phys_addr, size, dir);
    }
}

void hal_dma_sync_for_cpu(void *cpu_addr, uint64_t phys_addr, size_t size, hal_dma_direction_t dir) {
    if (g_hal_dma_ops && g_hal_dma_ops->sync_for_cpu) {
        g_hal_dma_ops->sync_for_cpu(cpu_addr, phys_addr, size, dir);
    }
}

int hal_dma_is_coherent(void) {
    if (g_hal_dma_ops && g_hal_dma_ops->is_coherent) {
        return g_hal_dma_ops->is_coherent();
    }
    return 1;
}
