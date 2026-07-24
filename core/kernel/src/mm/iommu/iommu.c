#include "mm/iommu.h"
#include "hal/hal_iommu.h"
#include "slab.h"
#include "console/console_core.h"

static const hal_iommu_ops_t *g_iommu_ops = NULL;
static bool g_iommu_available = false;

void hal_iommu_set_ops(const hal_iommu_ops_t *ops) {
    g_iommu_ops = ops;
}

const hal_iommu_ops_t *hal_iommu_get_ops(void) {
    return g_iommu_ops;
}

// Explicit stub for external HAL inclusion of null backend
extern const hal_iommu_ops_t null_iommu_ops;

int iommu_init(void) {
    // If no backend is set, fallback to the null backend
    if (!g_iommu_ops) {
        g_iommu_ops = &null_iommu_ops;
        console_write_raw("IOMMU: Initialized with NULL backend\n", 37);
    }

    // Initialize fault reporting via IRQ (hardcoded vector for demo)
    (void)iommu_fault_init(0x30);

    if (g_iommu_ops && g_iommu_ops->init) {
        int ret = g_iommu_ops->init();
        if (ret == 0 && g_iommu_ops->query_state) {
            iommu_state_t state = g_iommu_ops->query_state();
            if (state == IOMMU_STATE_ENABLED || state == IOMMU_STATE_BYPASS) {
                g_iommu_available = true;
            } else {
                g_iommu_available = false;
            }
        }
        return ret;
    }

    return -1;
}

bool iommu_available(void) {
    return g_iommu_available;
}

bool iommu_enabled(void) {
    if (g_iommu_ops && g_iommu_ops->query_state) {
        return g_iommu_ops->query_state() == IOMMU_STATE_ENABLED;
    }
    return false;
}

int iommu_get_dma_mode(iommu_device_t *dev, dma_translation_mode_t *mode_out) {
    if (!mode_out) return -1;

    if (!iommu_available()) {
        *mode_out = DMA_TRANSLATION_DIRECT;
        return 0;
    }

    if (!dev) {
        *mode_out = DMA_TRANSLATION_DIRECT; // Fallback
        return 0;
    }

    if (dev->mode == IOMMU_DEV_UNMANAGED) {
        *mode_out = DMA_TRANSLATION_DIRECT;
    } else if (dev->mode == IOMMU_DEV_IDENTITY) {
        *mode_out = DMA_TRANSLATION_IDENTITY;
    } else if (dev->mode == IOMMU_DEV_BYPASS) {
        *mode_out = DMA_TRANSLATION_DIRECT;
    } else if (dev->mode == IOMMU_DEV_BLOCKED) {
        *mode_out = DMA_TRANSLATION_BLOCKED;
    } else if (dev->mode == IOMMU_DEV_MANAGED) {
        *mode_out = DMA_TRANSLATION_IOMMU;
    } else {
        *mode_out = DMA_TRANSLATION_DIRECT;
    }

    return 0;
}
