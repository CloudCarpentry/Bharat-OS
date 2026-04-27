#include <bharat/uapi/safety/bh_safe_state.h>
#include <trace/bh_fast_trace.h>
#include <stdbool.h>

static bh_auto_safety_state_t g_current_safety_state = BH_AUTO_STATE_NORMAL;

bh_auto_safety_state_t bh_auto_get_safety_state(void) {
    return g_current_safety_state;
}

bool bh_auto_transition_safety_state(bh_auto_safety_state_t next_state) {
    bool allowed = false;

    if (g_current_safety_state == next_state) return true;

    switch (g_current_safety_state) {
        case BH_AUTO_STATE_NORMAL:
            if (next_state == BH_AUTO_STATE_DEGRADED || next_state == BH_AUTO_STATE_SAFE_STOP_REQUESTED) {
                allowed = true;
            }
            break;
        case BH_AUTO_STATE_DEGRADED:
            if (next_state == BH_AUTO_STATE_SAFE_STOP_REQUESTED) {
                allowed = true;
            }
            break;
        case BH_AUTO_STATE_SAFE_STOP_REQUESTED:
            if (next_state == BH_AUTO_STATE_SAFE_STOP_ACTIVE) {
                allowed = true;
            }
            break;
        case BH_AUTO_STATE_SAFE_STOP_ACTIVE:
            if (next_state == BH_AUTO_STATE_FAULT_LOCKDOWN) {
                allowed = true;
            }
            break;
        case BH_AUTO_STATE_FAULT_LOCKDOWN:
            allowed = false;
            break;
    }

    if (allowed) {
        g_current_safety_state = next_state;
        bh_fast_trace_emit(BH_TRACE_AUTO_SAFE_STATE_ENTER, (uint64_t)next_state, 0, 0);
    }

    return allowed;
}
