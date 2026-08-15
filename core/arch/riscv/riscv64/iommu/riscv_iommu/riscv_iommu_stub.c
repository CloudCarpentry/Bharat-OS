#include "hal/hal_iommu.h"

// Basic stub to allow compilation
static int rviommu_init(void) { return -1; }
static iommu_state_t rviommu_query_state(void) { return IOMMU_STATE_NONE; }

const hal_iommu_ops_t riscv_iommu_ops = {
    .init        = rviommu_init,
    .query_state = rviommu_query_state,
};
