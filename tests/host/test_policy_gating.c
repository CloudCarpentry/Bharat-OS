#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <stdbool.h>
#include <string.h>
#include <bharat/uapi/servicemgr/contract.h>
#include <bharat/uapi/system/policy_contract.h>

// Mock service registry
#define MAX_SERVICES 64
typedef struct {
    uint32_t service_id;
    uint32_t required_caps;
    sm_service_state_t state;
    bool in_use;
} mock_service_entry_t;

static mock_service_entry_t service_registry[MAX_SERVICES];

// Mock policymgr configuration
static bool g_mock_policymgr_available = true;
static bharat_policy_decision_t g_mock_policymgr_decision = POLICY_DECISION_ALLOW;

// Mock IPC call
int32_t bharat_ipc_call(int endpoint, const void *req_header, const void *req_payload,
                        void *resp_header, void *resp_payload, uint32_t rep_max_size) {
    (void)req_header;
    (void)rep_max_size;
    if (endpoint == 12) {
        if (!g_mock_policymgr_available) {
            return -8; // BHARAT_IPC_STATUS_ERR_TIMEOUT
        }

        const policy_req_query_service_t *req = (const policy_req_query_service_t *)req_payload;
        policy_resp_query_service_t *resp = (policy_resp_query_service_t *)resp_payload;

        // Mock policymgr decision logic
        if (req->service_id == 5) {
            resp->decision = POLICY_DECISION_DENY;
            resp->allowed_caps = 0;
        } else {
            resp->decision = g_mock_policymgr_decision;
            resp->allowed_caps = req->requested_caps;
        }

        // Mock a success response header flag
        struct { uint32_t flags; } *header = (void *)resp_header;
        header->flags = 0; // BHARAT_IPC_STATUS_OK

        return 0; // BHARAT_IPC_STATUS_OK
    }
    return -1;
}

// Last-known-good cache for policy decisions
typedef struct {
    uint32_t service_id;
    bharat_policy_decision_t decision;
    bool valid;
} policy_cache_entry_t;

static policy_cache_entry_t policy_cache[MAX_SERVICES];

// Function under test (extracted logic from servicemgr)
static bharat_policy_decision_t servicemgr_query_policy(uint32_t service_id, uint32_t caps) {
    struct { uint32_t flags; } req_header = {0};
    policy_req_query_service_t req = {
        .service_id = service_id,
        .requested_caps = caps
    };

    struct { uint32_t flags; } resp_header = {0};
    policy_resp_query_service_t resp = {0};

    int32_t st = bharat_ipc_call(12, &req_header, &req, &resp_header, &resp, sizeof(resp));
    if (st == 0 && resp_header.flags == 0) {
        for (int i = 0; i < MAX_SERVICES; i++) {
            if (!policy_cache[i].valid || policy_cache[i].service_id == service_id) {
                policy_cache[i].service_id = service_id;
                policy_cache[i].decision = resp.decision;
                policy_cache[i].valid = true;
                break;
            }
        }
        return resp.decision;
    }

    for (int i = 0; i < MAX_SERVICES; i++) {
        if (policy_cache[i].valid && policy_cache[i].service_id == service_id) {
            return policy_cache[i].decision;
        }
    }

    return POLICY_DECISION_ALLOW;
}

static int32_t servicemgr_handle_start_mock(uint32_t service_id) {
    for (int i = 0; i < MAX_SERVICES; i++) {
        if (service_registry[i].in_use && service_registry[i].service_id == service_id) {
            if (service_registry[i].state == SM_STATE_STOPPED) {
                bharat_policy_decision_t decision = servicemgr_query_policy(service_registry[i].service_id, service_registry[i].required_caps);

                if (decision == POLICY_DECISION_DENY) {
                    service_registry[i].state = SM_STATE_DENIED_BY_POLICY;
                    return -4; // BHARAT_IPC_STATUS_ERR_PERM
                }

                service_registry[i].state = SM_STATE_STARTING;
                return 0; // BHARAT_IPC_STATUS_OK
            }
        }
    }
    return -1;
}

int main() {
    memset(policy_cache, 0, sizeof(policy_cache));

    // Setup test registry
    service_registry[0] = (mock_service_entry_t){.service_id = 1, .required_caps = 0, .state = SM_STATE_STOPPED, .in_use = true}; // Normal service
    service_registry[1] = (mock_service_entry_t){.service_id = 5, .required_caps = 0, .state = SM_STATE_STOPPED, .in_use = true}; // Hardcoded denied service

    // Test 1: Normal service launch, policymgr allows. Expected behavior: Allowed and cached.
    g_mock_policymgr_available = true;
    g_mock_policymgr_decision = POLICY_DECISION_ALLOW;
    assert(servicemgr_handle_start_mock(1) == 0);
    assert(service_registry[0].state == SM_STATE_STARTING);
    assert(policy_cache[0].valid == true);
    assert(policy_cache[0].service_id == 1);
    assert(policy_cache[0].decision == POLICY_DECISION_ALLOW);

    // Test 2: Denied service launch (Service 5 is hardcoded to deny). Expected: Denied and cached.
    assert(servicemgr_handle_start_mock(5) == -4);
    assert(service_registry[1].state == SM_STATE_DENIED_BY_POLICY);
    assert(policy_cache[1].valid == true);
    assert(policy_cache[1].service_id == 5);
    assert(policy_cache[1].decision == POLICY_DECISION_DENY);

    // Test 3: Fallback caching. Policymgr crashes/unavailable, try to restart service 5.
    service_registry[1].state = SM_STATE_STOPPED;
    g_mock_policymgr_available = false; // Simulate crash
    assert(servicemgr_handle_start_mock(5) == -4); // Should read DENY from cache
    assert(service_registry[1].state == SM_STATE_DENIED_BY_POLICY);

    printf("All host policy gating tests passed.\n");
    return 0;
}
