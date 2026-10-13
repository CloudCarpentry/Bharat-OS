#include <bharat/coremgr/coremgr.h>
#include <stddef.h>

static bool is_valid_transition(coremgr_core_state_t current, coremgr_core_state_t next) {
    if (current == next) {
        return true; // No-op transition is considered valid (or can be ignored)
    }

    switch (current) {
        case CORE_STATE_UNKNOWN:
            return next == CORE_STATE_DISCOVERED;
        case CORE_STATE_DISCOVERED:
            return next == CORE_STATE_STARTING || next == CORE_STATE_FAILED || next == CORE_STATE_OFFLINE;
        case CORE_STATE_STARTING:
            return next == CORE_STATE_ONLINE || next == CORE_STATE_FAILED || next == CORE_STATE_OFFLINE;
        case CORE_STATE_ONLINE:
            return next == CORE_STATE_DEGRADED || next == CORE_STATE_OFFLINE || next == CORE_STATE_FAILED;
        case CORE_STATE_DEGRADED:
            return next == CORE_STATE_ONLINE || next == CORE_STATE_OFFLINE || next == CORE_STATE_FAILED;
        case CORE_STATE_OFFLINE:
            return next == CORE_STATE_STARTING || next == CORE_STATE_FAILED; // Re-start allowed
        case CORE_STATE_FAILED:
            return next == CORE_STATE_OFFLINE || next == CORE_STATE_STARTING; // Manual intervention to offline/restart
        default:
            return false;
    }
}

int coremgr_set_core_state(uint32_t core_id, coremgr_core_state_t new_state) {
    coremgr_core_info_t* info = coremgr_get_core_info_ptr(core_id);
    if (!info) {
        return -1;
    }

    if (!is_valid_transition(info->lifecycle_state, new_state)) {
        return -2; // Invalid transition
    }

    info->lifecycle_state = new_state;
    info->is_online = (new_state == CORE_STATE_ONLINE || new_state == CORE_STATE_DEGRADED);

    return 0;
}

int coremgr_set_transport_state(uint32_t core_id, coremgr_transport_state_t new_state) {
    coremgr_core_info_t* info = coremgr_get_core_info_ptr(core_id);
    if (!info) {
        return -1;
    }

    info->transport_state = new_state;
    if (new_state == CORE_TRANSPORT_FAILED && info->lifecycle_state == CORE_STATE_ONLINE) {
        coremgr_set_core_state(core_id, CORE_STATE_DEGRADED);
    }

    return 0;
}

int coremgr_record_failure(uint32_t core_id, uint32_t reason) {
    coremgr_core_info_t* info = coremgr_get_core_info_ptr(core_id);
    if (!info) {
        return -1;
    }

    info->last_failure_reason = reason;
    return coremgr_set_core_state(core_id, CORE_STATE_FAILED);
}
