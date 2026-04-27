#ifndef BHARAT_UAPI_SAFE_STATE_H
#define BHARAT_UAPI_SAFE_STATE_H

#include <stdint.h>

typedef enum bh_auto_safety_state {
    BH_AUTO_STATE_NORMAL = 0,
    BH_AUTO_STATE_DEGRADED,
    BH_AUTO_STATE_SAFE_STOP_REQUESTED,
    BH_AUTO_STATE_SAFE_STOP_ACTIVE,
    BH_AUTO_STATE_FAULT_LOCKDOWN,
} bh_auto_safety_state_t;

#endif // BHARAT_UAPI_SAFE_STATE_H
