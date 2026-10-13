#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "../state.h"

static void test_state_tracking_and_policy(void) {
    printf("Running test_state_tracking_and_policy...\n");
    faultmgr_state_init();

    faultmgr_req_report_fault_t req = {
        .domain_id = 1,
        .source_id = 10,
        .category = FAULT_CAT_CRASH,
        .severity = FAULT_SEV_WARNING,
        .incarnation_id = 1
    };

    faultmgr_recovery_action_t act1 = faultmgr_process_event(&req, 100);
    assert(act1 == FAULT_REC_RETRY); // First failure warning

    faultmgr_recovery_action_t act2 = faultmgr_process_event(&req, 200);
    assert(act2 == FAULT_REC_RETRY);

    req.severity = FAULT_SEV_ERROR;
    faultmgr_recovery_action_t act3 = faultmgr_process_event(&req, 300);
    assert(act3 == FAULT_REC_ISOLATE_SERVICE); // Error with 3 fails -> isolate (total fails = 3 now)

    // Total failures is now 4
    faultmgr_recovery_action_t act4 = faultmgr_process_event(&req, 400);
    assert(act4 == FAULT_REC_ISOLATE_SERVICE); // Error >= 3 fails -> isolate

    req.severity = FAULT_SEV_CRITICAL;
    faultmgr_recovery_action_t act5 = faultmgr_process_event(&req, 500);
    assert(act5 == FAULT_REC_ESCALATE_TO_SUPERVISOR); // Critical -> escalate

    printf("Passed.\n");
}

static void test_repeated_failure(void) {
    printf("Running test_repeated_failure...\n");
    faultmgr_state_init();

    faultmgr_req_report_fault_t req = {
        .domain_id = 2,
        .source_id = 20,
        .category = FAULT_CAT_REPEATED_FAILURE,
        .severity = FAULT_SEV_WARNING,
        .incarnation_id = 1
    };

    faultmgr_recovery_action_t act1 = faultmgr_process_event(&req, 100);
    // Repeated failure warning -> Isolate
    assert(act1 == FAULT_REC_ISOLATE_SERVICE);

    req.severity = FAULT_SEV_ERROR;
    faultmgr_recovery_action_t act2 = faultmgr_process_event(&req, 200);
    // Repeated failure error -> Escalate
    assert(act2 == FAULT_REC_ESCALATE_TO_SUPERVISOR);

    printf("Passed.\n");
}

static void test_stale_incarnation(void) {
    printf("Running test_stale_incarnation...\n");
    faultmgr_state_init();

    faultmgr_req_report_fault_t req1 = {
        .domain_id = 3,
        .source_id = 30,
        .category = FAULT_CAT_CRASH,
        .severity = FAULT_SEV_ERROR,
        .incarnation_id = 2
    };
    faultmgr_recovery_action_t act1 = faultmgr_process_event(&req1, 100);
    assert(act1 == FAULT_REC_RESTART_SERVICE);

    // Stale message
    faultmgr_req_report_fault_t req2 = {
        .domain_id = 3,
        .source_id = 30,
        .category = FAULT_CAT_CRASH,
        .severity = FAULT_SEV_ERROR,
        .incarnation_id = 1
    };
    faultmgr_recovery_action_t act2 = faultmgr_process_event(&req2, 200);
    assert(act2 == FAULT_REC_RESTART_SERVICE); // Returns previous action without updating state

    const fault_record_t* rec = faultmgr_get_record(3);
    assert(rec != NULL);
    assert(rec->failure_count == 1); // Should not have incremented

    printf("Passed.\n");
}

static void test_table_overflow(void) {
    printf("Running test_table_overflow...\n");
    faultmgr_state_init();

    for (uint32_t i = 1; i <= FAULTMGR_MAX_RECORDS + 10; ++i) {
        faultmgr_req_report_fault_t req = {
            .domain_id = i,
            .source_id = i,
            .category = FAULT_CAT_CRASH,
            .severity = FAULT_SEV_WARNING,
            .incarnation_id = 1
        };
        faultmgr_process_event(&req, i * 100);
    }

    // The oldest records (1 to 10) should have been evicted
    assert(faultmgr_get_record(1) == NULL);
    assert(faultmgr_get_record(10) == NULL);
    // 11 should be the oldest active now
    assert(faultmgr_get_record(11) != NULL);
    assert(faultmgr_get_record(FAULTMGR_MAX_RECORDS + 10) != NULL);

    printf("Passed.\n");
}

static void test_observation_window_decay(void) {
    printf("Running test_observation_window_decay...\n");
    faultmgr_state_init();

    faultmgr_req_report_fault_t req = {
        .domain_id = 4,
        .source_id = 40,
        .category = FAULT_CAT_CRASH,
        .severity = FAULT_SEV_ERROR,
        .incarnation_id = 1
    };

    // First fault at t=0
    faultmgr_recovery_action_t act1 = faultmgr_process_event(&req, 0);
    assert(act1 == FAULT_REC_RESTART_SERVICE);

    // Second fault at t=1s (within window)
    faultmgr_recovery_action_t act2 = faultmgr_process_event(&req, 1000000000ULL);
    assert(act2 == FAULT_REC_RESTART_SERVICE);

    // Third fault at t=7s (outside 5s window since last fault)
    // The previous 2 faults should decay, and this is treated as the first again.
    faultmgr_recovery_action_t act3 = faultmgr_process_event(&req, 7000000000ULL);
    assert(act3 == FAULT_REC_RESTART_SERVICE);

    const fault_record_t* rec = faultmgr_get_record(4);
    assert(rec != NULL);
    assert(rec->failure_count == 1); // Should have reset to 0 before incrementing to 1

    printf("Passed.\n");
}

int main(void) {
    test_state_tracking_and_policy();
    test_repeated_failure();
    test_stale_incarnation();
    test_table_overflow();
    test_observation_window_decay();

    printf("All faultmgr host tests passed.\n");
    return 0;
}
