#include "state.h"
#include <stddef.h>

static fault_record_t g_fault_table[FAULTMGR_MAX_RECORDS];
static uint32_t g_active_records = 0;

void faultmgr_state_init(void) {
    faultmgr_clear_state();
}

void faultmgr_clear_state(void) {
    for (int i = 0; i < FAULTMGR_MAX_RECORDS; ++i) {
        g_fault_table[i].active = false;
        g_fault_table[i].failure_count = 0;
    }
    g_active_records = 0;
}

const fault_record_t* faultmgr_get_record(uint32_t domain_id) {
    for (int i = 0; i < FAULTMGR_MAX_RECORDS; ++i) {
        if (g_fault_table[i].active && g_fault_table[i].domain_id == domain_id) {
            return &g_fault_table[i];
        }
    }
    return NULL;
}

static fault_record_t* find_or_create_record(uint32_t domain_id) {
    fault_record_t* empty_slot = NULL;
    fault_record_t* oldest_record = NULL;
    uint64_t oldest_time = UINT64_MAX;

    for (int i = 0; i < FAULTMGR_MAX_RECORDS; ++i) {
        if (!g_fault_table[i].active) {
            if (!empty_slot) empty_slot = &g_fault_table[i];
        } else {
            if (g_fault_table[i].domain_id == domain_id) {
                return &g_fault_table[i];
            }
            if (g_fault_table[i].latest_occurrence < oldest_time) {
                oldest_time = g_fault_table[i].latest_occurrence;
                oldest_record = &g_fault_table[i];
            }
        }
    }

    if (empty_slot) {
        return empty_slot;
    }

    // Deterministic overflow policy: evict oldest
    if (oldest_record) {
        return oldest_record;
    }

    return &g_fault_table[0]; // fallback
}

faultmgr_recovery_action_t faultmgr_process_event(const faultmgr_req_report_fault_t *req, uint64_t current_time) {
    if (!req) return FAULT_REC_NO_ACTION;

    fault_record_t *rec = find_or_create_record(req->domain_id);

    if (!rec->active || rec->domain_id != req->domain_id) {
        // new record or overwritten
        rec->active = true;
        rec->domain_id = req->domain_id;
        rec->source_id = req->source_id;
        rec->first_occurrence = current_time;
        rec->failure_count = 0;
        rec->highest_severity = req->severity;
        rec->incarnation_id = req->incarnation_id;
    }

    if (req->incarnation_id < rec->incarnation_id) {
        // Stale fault event, reject
        return rec->last_action; // Just return previous
    }
    rec->incarnation_id = req->incarnation_id;

    // Decay failures outside of observation window
    if (current_time > rec->latest_occurrence) {
        uint64_t elapsed = current_time - rec->latest_occurrence;
        if (elapsed > FAULTMGR_OBSERVATION_WINDOW_NS) {
            rec->failure_count = 0;
            rec->highest_severity = FAULT_SEV_INFO; // reset severity for new window
        }
    }

    rec->latest_occurrence = current_time;
    rec->failure_count++;
    rec->last_category = req->category;

    if (req->severity > rec->highest_severity) {
        rec->highest_severity = req->severity;
    }

    // Policy Evaluation
    faultmgr_recovery_action_t action = FAULT_REC_NO_ACTION;

    if (rec->highest_severity == FAULT_SEV_CRITICAL) {
        action = FAULT_REC_ESCALATE_TO_SUPERVISOR;
    } else if (rec->highest_severity == FAULT_SEV_ERROR) {
        if (rec->failure_count >= 3) {
            action = FAULT_REC_ISOLATE_SERVICE;
        } else {
            action = FAULT_REC_RESTART_SERVICE;
        }
    } else if (rec->highest_severity == FAULT_SEV_WARNING) {
        if (rec->failure_count >= 5) {
            action = FAULT_REC_ENTER_DEGRADED_MODE;
        } else {
            action = FAULT_REC_RETRY;
        }
    }

    if (req->category == FAULT_CAT_REPEATED_FAILURE && action < FAULT_REC_ESCALATE_TO_SUPERVISOR) {
        if (rec->highest_severity >= FAULT_SEV_ERROR) {
            action = FAULT_REC_ESCALATE_TO_SUPERVISOR;
        } else {
            action = FAULT_REC_ISOLATE_SERVICE;
        }
    }

    rec->last_action = action;
    return action;
}
