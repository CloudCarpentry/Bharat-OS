#ifndef BHARAT_LIB_RUNTIME_BACKEND_DISPATCH_H
#define BHARAT_LIB_RUNTIME_BACKEND_DISPATCH_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Include the standard taxonomy for feature classes
#include "uapi/capability/feature_class.h"
#include "uapi/capability/hw_caps.h"

typedef enum {
    BACKEND_TYPE_SOFTWARE_FALLBACK = 0,
    BACKEND_TYPE_GENERIC_HARDWARE,
    BACKEND_TYPE_VENDOR_HARDWARE
} backend_type_t;

typedef struct {
    uint32_t power_level;   // E.g., 0=battery saver, 100=max performance
    bool     safe_mode;     // True if system is in degraded/safe mode
    uint32_t qos_level;     // Target QoS required by the caller
} backend_dispatch_context_t;

/**
 * Common opaque backend interface.
 */
typedef struct backend_interface backend_interface_t;

typedef struct {
    const char             *name;
    bharat_feature_class_t  feature_class;
    backend_type_t          type;
    uint32_t                priority; // Higher is preferred

    // Lifecycle hooks
    int  (*init)(void);
    bool (*is_available)(const bharat_hw_caps_t *caps, const backend_dispatch_context_t *ctx);
    backend_interface_t* (*get_interface)(void);
} backend_provider_t;

/**
 * Register a new backend provider into the dispatch registry.
 * @param provider The provider definition.
 * @return 0 on success, negative on error.
 */
int backend_registry_add(const backend_provider_t *provider);

/**
 * Select the optimal backend based on capabilities, profile, and current context.
 * Always returns a software fallback if a hardware backend is absent or unavailable
 * due to safe mode / power constraints.
 *
 * @param feature_class The class of feature requested (e.g. CLASS_CRYPTO).
 * @param caps The system capability record.
 * @param ctx The current execution context (power state, QoS, safe mode).
 * @return A pointer to the selected backend provider, or NULL if none exists.
 */
const backend_provider_t *backend_dispatch_select(
    bharat_feature_class_t feature_class,
    const bharat_hw_caps_t *caps,
    const backend_dispatch_context_t *ctx);

#ifdef __cplusplus
}
#endif

#endif /* BHARAT_LIB_RUNTIME_BACKEND_DISPATCH_H */
