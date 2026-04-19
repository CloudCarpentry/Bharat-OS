#ifndef BHARAT_FAULT_DOMAIN_H
#define BHARAT_FAULT_DOMAIN_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Identifies an isolated fault domain.
 * Advanced features and backend accelerators must be mapped to a fault domain
 * so failures can be isolated, contained, and recovered without crashing the
 * whole system.
 */
typedef uint32_t fault_domain_id_t;

#define FAULT_DOMAIN_INVALID 0
#define FAULT_DOMAIN_GLOBAL  1

/**
 * Hint on how the fault domain manager should attempt to recover the domain.
 */
typedef enum {
    FAULT_RESTART_HINT_NONE = 0,
    FAULT_RESTART_HINT_RESTART_SERVICE,
    FAULT_RESTART_HINT_RESET_HARDWARE,
    FAULT_RESTART_HINT_FALLBACK_SOFTWARE,
    FAULT_RESTART_HINT_ISOLATE_PERMANENTLY
} fault_restart_hint_t;

/**
 * Represents a fault event passed to a fault manager (in services/).
 * The kernel routes the event; the service enforces the policy.
 */
typedef struct {
    fault_domain_id_t    domain_id;
    uint32_t             fault_reason;
    fault_restart_hint_t restart_hint;
} fault_event_t;

#ifdef __cplusplus
}
#endif

#endif /* BHARAT_FAULT_DOMAIN_H */
