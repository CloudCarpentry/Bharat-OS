#include "hal/hal_iommu.h"

// Basic stub to allow compilation
static int smmu_init(void) { return -1; }
static iommu_state_t smmu_query_state(void) { return IOMMU_STATE_NONE; }

const hal_iommu_ops_t smmu_iommu_ops = {
    .init        = smmu_init,
    .query_state = smmu_query_state,
};
