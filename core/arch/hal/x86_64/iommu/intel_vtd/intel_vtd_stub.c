#include "hal/hal_iommu.h"

// Basic stub to allow compilation
static int vtd_init(void) { return -1; }
static iommu_state_t vtd_query_state(void) { return IOMMU_STATE_NONE; }

const hal_iommu_ops_t vtd_iommu_ops = {
    .init        = vtd_init,
    .query_state = vtd_query_state,
};
