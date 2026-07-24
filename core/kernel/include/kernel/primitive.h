#ifndef BHARAT_KERNEL_PRIMITIVE_H
#define BHARAT_KERNEL_PRIMITIVE_H

#include <stdbool.h>
#include "kernel/status.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Kernel Primitive Classes.
 * Defines the core mechanism boundaries of the Bharat-OS kernel.
 */
typedef enum {
    BH_PRIMITIVE_SCHED,
    BH_PRIMITIVE_MEMORY,
    BH_PRIMITIVE_CAPABILITY,
    BH_PRIMITIVE_IPC,
    BH_PRIMITIVE_TIMER,
    BH_PRIMITIVE_FAULT,
    BH_PRIMITIVE_DMA,
    BH_PRIMITIVE_ACCEL,
    BH_PRIMITIVE_TELEMETRY,
    BH_PRIMITIVE_COUNT
} bh_kernel_primitive_class_t;

/**
 * Primitive Support Levels.
 * Indicates how a specific primitive is implemented on the current hardware/profile.
 */
typedef enum {
    BH_PRIMITIVE_UNSUPPORTED = 0,
    BH_PRIMITIVE_STUBBED,
    BH_PRIMITIVE_SOFTWARE_FALLBACK,
    BH_PRIMITIVE_HARDWARE_ASSISTED,
    BH_PRIMITIVE_HARDWARE_ENFORCED,
} bh_primitive_support_level_t;

/**
 * Check if a kernel primitive is available.
 *
 * @param primitive The primitive class to query.
 * @return true if the primitive is available (at any support level > UNSUPPORTED).
 */
bool bh_kernel_primitive_available(bh_kernel_primitive_class_t primitive);

/**
 * Get the support level for a kernel primitive.
 *
 * @param primitive The primitive class to query.
 * @return The support level for the primitive.
 */
bh_primitive_support_level_t bh_kernel_primitive_get_support_level(bh_kernel_primitive_class_t primitive);

#include "hal/hal_hw_caps.h"

/**
 * Initialize the kernel primitive registry.
 *
 * @param caps The discovered hardware capabilities.
 * @return K_OK on success, or an error code.
 */
kstatus_t bh_kernel_primitive_registry_init(const hal_hw_caps_t *caps);

#ifdef __cplusplus
}
#endif

#endif /* BHARAT_KERNEL_PRIMITIVE_H */
