#ifndef BHARAT_HAL_INTERNAL_H
#define BHARAT_HAL_INTERNAL_H

#include "hal/hal_hw_caps.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Architecture-specific hardware capability discovery.
 * Implemented in core/arch/[arch]/hw_caps.c
 */
void arch_discover_hw_caps(void);

/**
 * Internal HAL-only API to set the discovered capabilities.
 * Implemented in core/hal/common/hw_caps.c
 */
void hal_set_internal_hw_caps(const hal_hw_caps_t *caps);

#ifdef __cplusplus
}
#endif

#endif /* BHARAT_HAL_INTERNAL_H */
