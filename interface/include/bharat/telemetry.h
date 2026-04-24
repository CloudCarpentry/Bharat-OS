#ifndef BHARAT_KERNEL_TELEMETRY_H
#define BHARAT_KERNEL_TELEMETRY_H

#include <stdint.h>
#include <stdbool.h>

/* Use relative path or standard path depending on the build system setup.
 * Often uapi/ headers are included directly via include paths */
#include "system/telemetry.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file telemetry.h
 * @brief Kernel-internal API for exporting telemetry data.
 *
 * This API provides hooks for kernel subsystems and drivers to register
 * counters and emit events. It bridges internal kernel state to the stable
 * UAPI structures, making them accessible to user-space services (e.g., diag).
 *
 * Note: This layer purely manages emission and registration mechanisms.
 * Policy (such as routing, persistence, and complex filtering) is delegated
 * entirely to user-space services.
 */

/* --------------------------------------------------------------------------
 * Counters
 * -------------------------------------------------------------------------- */

/**
 * Register a new telemetry counter.
 *
 * @param desc Pointer to the counter description structure.
 * @return 0 on success, or a negative error code.
 */
int kernel_telemetry_register_counter(const bharat_counter_desc_t *desc);

/**
 * Update the value of a telemetry counter.
 *
 * @param counter_id The ID of the counter to update.
 * @param value The new value for the counter.
 * @return 0 on success, or a negative error code.
 */
int kernel_telemetry_update_counter(uint32_t counter_id, uint64_t value);

/**
 * Increment a telemetry counter by a specific amount.
 *
 * @param counter_id The ID of the counter to increment.
 * @param amount The amount to increment by.
 * @return 0 on success, or a negative error code.
 */
int kernel_telemetry_inc_counter(uint32_t counter_id, uint64_t amount);


/* --------------------------------------------------------------------------
 * Events
 * -------------------------------------------------------------------------- */

/**
 * Emit a telemetry event.
 *
 * @param event Pointer to the populated event structure. The timestamp
 *              will be automatically populated if it is 0.
 * @return 0 on success, or a negative error code.
 */
int kernel_telemetry_emit_event(bharat_telemetry_event_t *event);

/**
 * Helper to quickly emit a simple event with no payload.
 *
 * @param kind Event kind.
 * @param severity Event severity.
 * @param source_id Subsystem or domain ID.
 * @return 0 on success, or a negative error code.
 */
int kernel_telemetry_emit_simple_event(uint32_t kind, uint32_t severity, uint32_t source_id);

#ifdef __cplusplus
}
#endif

#endif /* BHARAT_KERNEL_TELEMETRY_H */