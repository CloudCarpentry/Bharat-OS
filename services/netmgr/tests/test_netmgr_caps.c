#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "../include/capability_checks.h"
#include "../include/ipc_dispatch.h"
#include "../include/ipc_auth.h"
#include <bharat/cap/cap.h>
#include <bharat/cap/cap_validate.h>

// Mock interface functions used by ipc_dispatch
#include "../include/interface_table.h"
#include "../include/address_table.h"
#include "../include/route_table.h"
#include "../include/neighbor_cache.h"
#include "../include/driver_policy.h"
#include "../include/driver_health.h"

// Stubs for the dependencies
void netmgr_iface_table_init(void) {}
void netmgr_addr_table_init(void) {}
void netmgr_route_table_init(void) {}
void netmgr_neighbor_cache_init(void) {}
void netmgr_driver_policy_init(void) {}
void netmgr_driver_health_init(void) {}

netmgr_status_t netmgr_iface_create(const char *name, const uint8_t *mac, uint32_t mtu, net_if_id_t *out_id) { return NETMGR_STATUS_OK; }
netmgr_status_t netmgr_iface_delete(net_if_id_t if_id) { return NETMGR_STATUS_OK; }
netmgr_status_t netmgr_iface_set_admin_state(net_if_id_t if_id, bool admin_up) { return NETMGR_STATUS_OK; }
netmgr_iface_t* netmgr_iface_get(net_if_id_t if_id) {
    static netmgr_iface_t mock_iface;
    return &mock_iface;
}
netmgr_status_t netmgr_addr_add(net_if_id_t if_id, net_af_t af, const uint8_t *addr, uint8_t prefix_len) { return NETMGR_STATUS_OK; }
netmgr_status_t netmgr_addr_remove(net_if_id_t if_id, net_af_t af, const uint8_t *addr, uint8_t prefix_len) { return NETMGR_STATUS_OK; }
netmgr_status_t netmgr_route_add(net_if_id_t if_id, net_af_t af, const uint8_t *dest, const uint8_t *mask, const uint8_t *gateway, uint32_t metric) { return NETMGR_STATUS_OK; }
netmgr_status_t netmgr_route_remove(net_af_t af, const uint8_t *dest, const uint8_t *mask) { return NETMGR_STATUS_OK; }
netmgr_status_t netmgr_neighbor_flush(net_if_id_t if_id) { return NETMGR_STATUS_OK; }
netmgr_status_t netmgr_driver_health_register(net_if_id_t if_id) { return NETMGR_STATUS_OK; }
netmgr_status_t netmgr_driver_health_unregister(net_if_id_t if_id) { return NETMGR_STATUS_OK; }
netmgr_status_t netmgr_driver_health_request_restart(net_if_id_t if_id) { return NETMGR_STATUS_OK; }

// Dummy DB for fake validator
static struct {
    uint64_t handle;
    bharat_cap_object_type_t object_type;
    uint64_t object_id;
    uint64_t rights;
    bharat_cap_scope_kind_t scope_kind;
    uint64_t scope_id;
    bool revoked;
    bool stale;
} mock_caps[10];

static int num_mock_caps = 0;

static void add_mock_cap(uint64_t handle, bharat_cap_object_type_t obj_type, uint64_t obj_id, uint64_t rights, bharat_cap_scope_kind_t scope_kind, uint64_t scope_id, bool revoked, bool stale) {
    mock_caps[num_mock_caps].handle = handle;
    mock_caps[num_mock_caps].object_type = obj_type;
    mock_caps[num_mock_caps].object_id = obj_id;
    mock_caps[num_mock_caps].rights = rights;
    mock_caps[num_mock_caps].scope_kind = scope_kind;
    mock_caps[num_mock_caps].scope_id = scope_id;
    mock_caps[num_mock_caps].revoked = revoked;
    mock_caps[num_mock_caps].stale = stale;
    num_mock_caps++;
}

static bharat_cap_status_t fake_cap_validate(
    bharat_cap_handle_t handle,
    bharat_cap_object_type_t expected_object_type,
    uint64_t expected_object_id,
    uint64_t required_rights,
    const bharat_cap_scope_t *required_scope,
    bharat_cap_validation_result_t *out_result)
{
    if (out_result) {
        memset(out_result, 0, sizeof(*out_result));
    }

    if (handle == BHARAT_CAP_INVALID_HANDLE) {
        if (out_result) out_result->status = BHARAT_CAP_INVALID;
        return BHARAT_CAP_INVALID;
    }

    int found = -1;
    for (int i = 0; i < num_mock_caps; i++) {
        if (mock_caps[i].handle == handle) {
            found = i;
            break;
        }
    }

    if (found == -1) {
        if (out_result) out_result->status = BHARAT_CAP_NOT_FOUND;
        return BHARAT_CAP_NOT_FOUND;
    }

    if (mock_caps[found].revoked) {
        if (out_result) out_result->status = BHARAT_CAP_REVOKED;
        return BHARAT_CAP_REVOKED;
    }

    if (mock_caps[found].stale) {
        if (out_result) out_result->status = BHARAT_CAP_STALE;
        return BHARAT_CAP_STALE;
    }

    if (mock_caps[found].object_type != expected_object_type) {
        if (out_result) out_result->status = BHARAT_CAP_OBJECT_MISMATCH;
        return BHARAT_CAP_OBJECT_MISMATCH;
    }

    // Object ID is checked depending on the scope

    if ((mock_caps[found].rights & required_rights) != required_rights) {
        if (out_result) out_result->status = BHARAT_CAP_RIGHTS_DENIED;
        return BHARAT_CAP_RIGHTS_DENIED;
    }

    if (required_scope) {
        if (mock_caps[found].scope_kind == BHARAT_CAP_SCOPE_GLOBAL) {
            // Global covers anything
        } else if (mock_caps[found].scope_kind == required_scope->kind) {
            if (mock_caps[found].scope_id != required_scope->scope_id) {
                if (out_result) out_result->status = BHARAT_CAP_SCOPE_DENIED;
                return BHARAT_CAP_SCOPE_DENIED;
            }
        } else {
            if (out_result) out_result->status = BHARAT_CAP_SCOPE_DENIED;
            return BHARAT_CAP_SCOPE_DENIED;
        }
    }

    if (out_result) {
        out_result->allowed = true;
        out_result->status = BHARAT_CAP_OK;
    }

    return BHARAT_CAP_OK;
}

int main(void) {
    bharat_cap_set_validate_backend_for_tests(fake_cap_validate);

    // Setup mock caps
    // Handle 1: Admin global
    add_mock_cap(1, BHARAT_CAP_OBJ_SERVICE, 0, BHARAT_CAP_RIGHT_NET_ADMIN, BHARAT_CAP_SCOPE_GLOBAL, 0, false, false);

    // Handle 2: Read stats on specific iface (iface 5)
    add_mock_cap(2, BHARAT_CAP_OBJ_NET_IFACE, 5, BHARAT_CAP_RIGHT_NET_READ_STATS, BHARAT_CAP_SCOPE_OBJECT, 5, false, false);

    // Handle 3: Revoked admin cap
    add_mock_cap(3, BHARAT_CAP_OBJ_SERVICE, 0, BHARAT_CAP_RIGHT_NET_ADMIN, BHARAT_CAP_SCOPE_GLOBAL, 0, true, false);

    // Handle 4: Stale read stats cap
    add_mock_cap(4, BHARAT_CAP_OBJ_NET_IFACE, 5, BHARAT_CAP_RIGHT_NET_READ_STATS, BHARAT_CAP_SCOPE_OBJECT, 5, false, true);

    netmgr_ipc_req_t req;
    netmgr_ipc_res_t res;
    bharat_ipc_msg_header_t hdr;

    // Test 1: No capability -> should fail with ERR_PERM
    memset(&req, 0, sizeof(req)); memset(&res, 0, sizeof(res)); memset(&hdr, 0, sizeof(hdr));
    req.opcode = NETMGR_OP_CREATE_IFACE;
    hdr.opcode = NETMGR_OP_CREATE_IFACE;
    hdr.interface_version = 1;
    hdr.payload_size = sizeof(struct netmgr_req_create_iface);
    hdr.capability_transfer = 0; // Invalid
    netmgr_ipc_handle_request(&hdr, &req, &res);
    if (res.status != NETMGR_STATUS_ERR_PERM) {
        printf("Test 1 failed, got status %d\n", res.status);
        fflush(stdout);
    }
    assert(res.status == NETMGR_STATUS_ERR_PERM);

    // Test 2: Valid capability (Admin global) -> should succeed
    memset(&req, 0, sizeof(req)); memset(&res, 0, sizeof(res)); memset(&hdr, 0, sizeof(hdr));
    req.opcode = NETMGR_OP_CREATE_IFACE;
    hdr.opcode = NETMGR_OP_CREATE_IFACE;
    hdr.capability_transfer = 1;
    hdr.payload_size = sizeof(struct netmgr_req_create_iface);
    hdr.interface_version = 1; // Needs to pass contract_validate
    netmgr_ipc_handle_request(&hdr, &req, &res);
    if (res.status != NETMGR_STATUS_OK) {
        printf("Test 2 failed, got status %d\n", res.status);
    }
    assert(res.status == NETMGR_STATUS_OK);

    // Test 3: Missing right (Iface config required, we have read stats) -> should fail
    memset(&req, 0, sizeof(req)); memset(&res, 0, sizeof(res)); memset(&hdr, 0, sizeof(hdr));
    req.opcode = NETMGR_OP_SET_ADMIN_STATE;
    hdr.opcode = NETMGR_OP_SET_ADMIN_STATE;
    req.u.set_admin_state.if_id = 5;
    hdr.capability_transfer = 2; // Read stats only
    hdr.interface_version = 1;
    hdr.payload_size = sizeof(struct netmgr_req_set_admin_state);
    netmgr_ipc_handle_request(&hdr, &req, &res);
    assert(res.status == NETMGR_STATUS_ERR_PERM);

    // Test 4: Correct right and scope (Read stats on iface 5) -> should succeed
    memset(&req, 0, sizeof(req)); memset(&res, 0, sizeof(res)); memset(&hdr, 0, sizeof(hdr));
    req.opcode = NETMGR_OP_QUERY_STATS;
    hdr.opcode = NETMGR_OP_QUERY_STATS;
    req.u.query_stats.if_id = 5;
    hdr.capability_transfer = 2;
    hdr.interface_version = 1;
    hdr.payload_size = sizeof(struct netmgr_req_iface_id);
    netmgr_ipc_handle_request(&hdr, &req, &res);
    if (res.status != NETMGR_STATUS_OK) {
        printf("Test 4 failed, got status %d\n", res.status);
        fflush(stdout);
    }
    assert(res.status == NETMGR_STATUS_OK);

    // Test 5: Wrong scope (Read stats on iface 5, try on iface 6) -> should fail
    memset(&req, 0, sizeof(req)); memset(&res, 0, sizeof(res)); memset(&hdr, 0, sizeof(hdr));
    req.opcode = NETMGR_OP_QUERY_STATS;
    hdr.opcode = NETMGR_OP_QUERY_STATS;
    req.u.query_stats.if_id = 6;
    hdr.capability_transfer = 2;
    hdr.interface_version = 1;
    hdr.payload_size = sizeof(struct netmgr_req_iface_id);
    netmgr_ipc_handle_request(&hdr, &req, &res);
    assert(res.status == NETMGR_STATUS_ERR_PERM);

    // Test 6: Revoked capability -> should fail
    memset(&req, 0, sizeof(req)); memset(&res, 0, sizeof(res)); memset(&hdr, 0, sizeof(hdr));
    req.opcode = NETMGR_OP_CREATE_IFACE;
    hdr.opcode = NETMGR_OP_CREATE_IFACE;
    hdr.capability_transfer = 3; // Revoked admin cap
    hdr.interface_version = 1;
    hdr.payload_size = sizeof(struct netmgr_req_create_iface);
    netmgr_ipc_handle_request(&hdr, &req, &res);
    assert(res.status == NETMGR_STATUS_ERR_PERM);

    // Test 7: Stale capability -> should fail
    memset(&req, 0, sizeof(req)); memset(&res, 0, sizeof(res)); memset(&hdr, 0, sizeof(hdr));
    req.opcode = NETMGR_OP_QUERY_STATS;
    hdr.opcode = NETMGR_OP_QUERY_STATS;
    req.u.query_stats.if_id = 5;
    hdr.capability_transfer = 4; // Stale read stats cap
    hdr.interface_version = 1;
    hdr.payload_size = sizeof(struct netmgr_req_iface_id);
    netmgr_ipc_handle_request(&hdr, &req, &res);
    assert(res.status == NETMGR_STATUS_ERR_PERM);

    // Test 8: Invalid OP -> should fail with INVAL, not PERM (since it falls to default)
    memset(&req, 0, sizeof(req)); memset(&res, 0, sizeof(res)); memset(&hdr, 0, sizeof(hdr));
    req.opcode = NETMGR_OP_INVALID;
    hdr.interface_version = 1;
    hdr.capability_transfer = 0;
    netmgr_ipc_handle_request(&hdr, &req, &res);
    assert(res.status == NETMGR_STATUS_ERR_INVAL);

    // Test 9: End-to-end Read-only Request (Query Interface State)
    memset(&req, 0, sizeof(req)); memset(&res, 0, sizeof(res)); memset(&hdr, 0, sizeof(hdr));
    req.opcode = NETMGR_OP_QUERY_STATS;
    hdr.opcode = NETMGR_OP_QUERY_STATS;
    req.u.query_stats.if_id = 5;
    hdr.capability_transfer = 2; // Read stats right
    hdr.interface_version = 1;
    hdr.payload_size = sizeof(struct netmgr_req_iface_id);
    netmgr_ipc_handle_request(&hdr, &req, &res);
    assert(res.status == NETMGR_STATUS_OK);

    return 0;
}
