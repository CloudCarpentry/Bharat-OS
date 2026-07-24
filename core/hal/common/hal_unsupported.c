#include "hal/hal_capabilities.h"
#include "kernel/status.h"

/* Common HAL status code for unsupported operations */
kstatus_t hal_unsupported_op(void) {
    return K_ERR_UNSUPPORTED;
}

/**
 * Validates reported architecture capabilities against a set of required features.
 */
bool hal_validate_arch_capabilities(const hal_arch_capabilities_t *caps,
                                   const hal_arch_capabilities_t *required) {
    if (!caps || !required) return false;

    if (caps->support_level < required->support_level) return false;
    if (caps->memory_model < required->memory_model) return false;
    if (required->has_smp && !caps->has_smp) return false;
    if (required->has_irq_controller && !caps->has_irq_controller) return false;
    if (required->has_monotonic_timer && !caps->has_monotonic_timer) return false;
    if (required->has_iommu && !caps->has_iommu) return false;

    return true;
}
