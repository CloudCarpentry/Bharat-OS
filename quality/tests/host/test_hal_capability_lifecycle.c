#include <assert.h>
#include <string.h>

#include "arch/arch_cpu_caps.h"
#include "arch/common/cpu_caps_state.h"
#include "hal/hal_cpu_features.h"
#include "hal/hal_discovery.h"
#include "hal/hal_hw_caps.h"
#include "hal/hal_internal.h"
#include "hal/hal_tlb.h"
#include "kernel/primitive.h"
#include "kernel/primitive_caps.h"

static uint32_t g_cpu_id;
static system_discovery_t g_discovery;
static hal_tlb_caps_t g_tlb_caps;

uint32_t hal_cpu_get_id(void) { return g_cpu_id; }
system_discovery_t *hal_get_system_discovery(void) { return &g_discovery; }
const hal_tlb_caps_t *hal_tlb_caps(void) { return &g_tlb_caps; }
void arch_cpu_caps_export_hal_features(const arch_cpu_caps_record_t *caps, void *out) {
    (void)caps;
    (void)out;
}

int main(void) {
    hal_cpu_feature_set_t features = {0};
    hal_hw_caps_t raw = {.has_mmu = false, .has_mpu = true, .has_dma_coherent = false};
    arch_cpu_caps_record_t empty = {0};
    arch_cpu_caps_record_t cpu0 = {0};
    arch_cpu_caps_record_t cpu1 = {0};

    assert(!hal_cpu_feature_set_system(HAL_CPU_FEATURE_SCOPE_ALL, &features));
    assert(arch_cpu_caps_system_finalize() == K_ERR_IN_PROGRESS);

    cpu_caps_state_set_boot(&empty);
    assert(arch_cpu_caps_system_finalize() == K_OK);
    memset(&features, 0xff, sizeof(features));
    assert(hal_cpu_feature_set_system(HAL_CPU_FEATURE_SCOPE_ALL, &features));
    assert(features.usable_bits[0] == 0U);

    arch_cpu_caps_set(&cpu0.raw, ARCH_CPU_FEAT_COMMON_VECTOR);
    arch_cpu_caps_set(&cpu0.usable, ARCH_CPU_FEAT_COMMON_VECTOR);
    cpu_caps_state_set_boot(&cpu0);
    cpu_caps_state_set_ap(1U, &cpu1);
    assert(arch_cpu_caps_system_finalize() == K_OK);
    assert(arch_cpu_caps_system_finalize() == K_ERR_BAD_STATE);
    assert(!hal_cpu_has_system_feature_all(HAL_CPU_FEATURE_VECTOR));
    assert(hal_cpu_has_system_feature_any(HAL_CPU_FEATURE_VECTOR));
    g_cpu_id = 0U;
    assert(hal_cpu_has_feature_current(HAL_CPU_FEATURE_VECTOR));
    g_cpu_id = 1U;
    assert(!hal_cpu_has_feature_current(HAL_CPU_FEATURE_VECTOR));

    assert(hal_hw_caps_publish_cpu() == K_ERR_BAD_STATE);
    assert(hal_hw_caps_publish_raw(&raw) == K_OK);
    assert(hal_hw_caps_state() == HAL_CAPS_RAW_DISCOVERED);
    assert(bh_kernel_primitive_registry_init(hal_get_internal_hw_caps()) == K_ERR_IN_PROGRESS);
    assert(hal_hw_caps_publish_cpu() == K_OK);
    assert(hal_hw_caps_state() == HAL_CAPS_CPU_FINALIZED);
    g_discovery.topology.cpu_count = 48U;
    g_discovery.iommu_count = 0U;
    assert(hal_hw_caps_finalize() == K_OK);
    assert(hal_hw_caps_is_frozen());
    assert(!hal_get_internal_hw_caps()->has_iommu);
    assert(!hal_get_internal_hw_caps()->has_dma_coherent);
    assert(!hal_get_internal_hw_caps()->has_mmu);
    assert(hal_get_internal_hw_caps()->has_mpu);
    assert(hal_get_internal_hw_caps()->max_cpus == 48U);
    assert(hal_set_internal_hw_caps(&raw) == K_ERR_BAD_STATE);
    assert(hal_hw_caps_finalize() == K_ERR_BAD_STATE);

    assert(bh_kernel_primitive_registry_init(hal_get_internal_hw_caps()) == K_OK);
    assert(bh_kernel_primitive_registry_init(hal_get_internal_hw_caps()) == K_ERR_BAD_STATE);
    assert(!bh_kprim_has(BH_KPRIM_CAP_ATOMIC_64));
    return 0;
}
