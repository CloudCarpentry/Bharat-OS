#include <stdio.h>
#include <assert.h>
#include <bharat/uapi/servicemgr/health.h>

void test_health_payload_size(void) {
    printf("Testing payload size limits...\n");
    assert(SM_HEALTH_UPDATE_MAX_PAYLOAD_SIZE == sizeof(sm_req_health_update_t));
    assert(sizeof(sm_req_health_update_t) > 0);
    // Arbitrary reasonable bound check to make sure it doesn't get too bloated over IPC
    assert(sizeof(sm_req_health_update_t) < 512);
    printf("test_health_payload_size PASSED\n");
}

void test_health_stale_heartbeat(void) {
    printf("Testing stale heartbeat detection logic (simulated)...\n");
    sm_req_health_update_t last_update;
    last_update.incarnation_id = 1;
    last_update.heartbeat_sequence = 100;

    sm_req_health_update_t new_update;

    // Case 1: normal progression
    new_update.incarnation_id = 1;
    new_update.heartbeat_sequence = 101;
    assert((new_update.incarnation_id > last_update.incarnation_id) ||
           (new_update.incarnation_id == last_update.incarnation_id && new_update.heartbeat_sequence > last_update.heartbeat_sequence));

    // Case 2: stale sequence
    new_update.incarnation_id = 1;
    new_update.heartbeat_sequence = 99;
    assert(!((new_update.incarnation_id > last_update.incarnation_id) ||
           (new_update.incarnation_id == last_update.incarnation_id && new_update.heartbeat_sequence > last_update.heartbeat_sequence)));

    // Case 3: new incarnation restarts sequence
    new_update.incarnation_id = 2;
    new_update.heartbeat_sequence = 1;
    assert((new_update.incarnation_id > last_update.incarnation_id) ||
           (new_update.incarnation_id == last_update.incarnation_id && new_update.heartbeat_sequence > last_update.heartbeat_sequence));

    printf("test_health_stale_heartbeat PASSED\n");
}

void test_health_state_transitions(void) {
    printf("Testing logical state transitions...\n");
    sm_health_state_t state1 = SM_HEALTH_STATE_DECLARED;
    sm_health_state_t state2 = SM_HEALTH_STATE_SPAWNED;
    sm_health_state_t state3 = SM_HEALTH_STATE_READY;

    // Simple ordinal checks ensuring states follow progression
    assert(state1 < state2);
    assert(state2 < state3);

    printf("test_health_state_transitions PASSED\n");
}

int main(void) {
    test_health_payload_size();
    test_health_stale_heartbeat();
    test_health_state_transitions();
    printf("All Health tests passed!\n");
    return 0;
}
