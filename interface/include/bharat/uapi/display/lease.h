#ifndef BHARAT_UAPI_DISPLAY_LEASE_H
#define BHARAT_UAPI_DISPLAY_LEASE_H

#include <stdint.h>

/**
 * Display Lease states for the Display Broker.
 */
typedef enum {
    BHARAT_DISPLAY_LEASE_STATE_INACTIVE = 0,
    BHARAT_DISPLAY_LEASE_STATE_ACTIVE = 1,
    BHARAT_DISPLAY_LEASE_STATE_REVOKING = 2,
    BHARAT_DISPLAY_LEASE_STATE_REVOKED = 3,
    BHARAT_DISPLAY_LEASE_STATE_EXPIRED = 4,
} bharat_display_lease_state_t;

/**
 * Display rights for the granular service capability model.
 */
#define BHARAT_DISPLAY_RIGHT_NONE    0x00000000u
#define BHARAT_DISPLAY_RIGHT_LEASE   0x00000001u
#define BHARAT_DISPLAY_RIGHT_PRESENT 0x00000002u
#define BHARAT_DISPLAY_RIGHT_MODESET 0x00000004u
#define BHARAT_DISPLAY_RIGHT_READ    0x00000008u
#define BHARAT_DISPLAY_RIGHT_WRITE   0x00000010u

typedef uint32_t bharat_display_lease_id_t;
#define BHARAT_DISPLAY_LEASE_INVALID ((bharat_display_lease_id_t)0)

#endif /* BHARAT_UAPI_DISPLAY_LEASE_H */
