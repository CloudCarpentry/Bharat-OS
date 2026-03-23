#include <assert.h>
#include <string.h>
#include "../src/capability_checks.h"
#include "../src/ipc_dispatch.h"
#include <bharat/cap/cap.h>

// Mock interface functions used by ipc_dispatch
#include "../src/interface_table.h"
#include "../src/address_table.h"
#include "../src/route_table.h"
#include "../src/neighbor_cache.h"
#include "../src/driver_policy.h"
#include "../src/driver_health.h"

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
netmgr_iface_t* netmgr_iface_get(net_if_id_t if_id) { return NULL; }
netmgr_status_t netmgr_addr_add(net_if_id_t if_id, net_af_t af, const uint8_t *addr, uint8_t prefix_len) { return NETMGR_STATUS_OK; }
netmgr_status_t netmgr_addr_remove(net_if_id_t if_id, net_af_t af, const uint8_t *addr, uint8_t prefix_len) { return NETMGR_STATUS_OK; }
netmgr_status_t netmgr_route_add(net_if_id_t if_id, net_af_t af, const uint8_t *dest, const uint8_t *mask, const uint8_t *gateway, uint32_t metric) { return NETMGR_STATUS_OK; }
netmgr_status_t netmgr_route_remove(net_af_t af, const uint8_t *dest, const uint8_t *mask) { return NETMGR_STATUS_OK; }
netmgr_status_t netmgr_neighbor_flush(net_if_id_t if_id) { return NETMGR_STATUS_OK; }
netmgr_status_t netmgr_driver_health_register(net_if_id_t if_id) { return NETMGR_STATUS_OK; }
netmgr_status_t netmgr_driver_health_unregister(net_if_id_t if_id) { return NETMGR_STATUS_OK; }
netmgr_status_t netmgr_driver_health_request_restart(net_if_id_t if_id) { return NETMGR_STATUS_OK; }


int main(void) {
    netmgr_ipc_req_t req;
    netmgr_ipc_res_t res;
    bharat_ipc_msg_header_t hdr;

    memset(&req, 0, sizeof(req));
    memset(&res, 0, sizeof(res));
    memset(&hdr, 0, sizeof(hdr));

    req.opcode = NETMGR_OP_CREATE_IFACE;

    // Test 1: No capability -> should fail with ERR_PERM
    hdr.capability_transfer = 0; // Invalid
    netmgr_ipc_handle_request(&hdr, &req, &res);
    assert(res.status == NETMGR_STATUS_ERR_PERM);

    // Test 2: Valid capability -> should succeed (or at least not fail with ERR_PERM)
    hdr.capability_transfer = 1; // Valid mock cap
    netmgr_ipc_handle_request(&hdr, &req, &res);
    assert(res.status == NETMGR_STATUS_OK);

    // Test 3: Other operation (e.g. NETMGR_OP_SET_ADMIN_STATE) with no capability -> should fail
    req.opcode = NETMGR_OP_SET_ADMIN_STATE;
    hdr.capability_transfer = 0;
    netmgr_ipc_handle_request(&hdr, &req, &res);
    assert(res.status == NETMGR_STATUS_ERR_PERM);

    // Test 4: DELETE_IFACE without capability -> should fail
    req.opcode = NETMGR_OP_DELETE_IFACE;
    hdr.capability_transfer = 0;
    netmgr_ipc_handle_request(&hdr, &req, &res);
    assert(res.status == NETMGR_STATUS_ERR_PERM);

    // Test 5: QUERY_STATS without capability -> should fail
    req.opcode = NETMGR_OP_QUERY_STATS;
    hdr.capability_transfer = 0;
    netmgr_ipc_handle_request(&hdr, &req, &res);
    assert(res.status == NETMGR_STATUS_ERR_PERM);

    // Test 6: ADD_ADDR without capability -> should fail
    req.opcode = NETMGR_OP_ADD_ADDR;
    hdr.capability_transfer = 0;
    netmgr_ipc_handle_request(&hdr, &req, &res);
    assert(res.status == NETMGR_STATUS_ERR_PERM);

    // Test 7: REMOVE_ADDR without capability -> should fail
    req.opcode = NETMGR_OP_REMOVE_ADDR;
    hdr.capability_transfer = 0;
    netmgr_ipc_handle_request(&hdr, &req, &res);
    assert(res.status == NETMGR_STATUS_ERR_PERM);

    // Test 8: ADD_ROUTE without capability -> should fail
    req.opcode = NETMGR_OP_ADD_ROUTE;
    hdr.capability_transfer = 0;
    netmgr_ipc_handle_request(&hdr, &req, &res);
    assert(res.status == NETMGR_STATUS_ERR_PERM);

    // Test 9: REMOVE_ROUTE without capability -> should fail
    req.opcode = NETMGR_OP_REMOVE_ROUTE;
    hdr.capability_transfer = 0;
    netmgr_ipc_handle_request(&hdr, &req, &res);
    assert(res.status == NETMGR_STATUS_ERR_PERM);

    // Test 10: NEIGHBOR_FLUSH without capability -> should fail
    req.opcode = NETMGR_OP_NEIGHBOR_FLUSH;
    hdr.capability_transfer = 0;
    netmgr_ipc_handle_request(&hdr, &req, &res);
    assert(res.status == NETMGR_STATUS_ERR_PERM);

    // Test 11: RESTART_DRIVER without capability -> should fail
    req.opcode = NETMGR_OP_RESTART_DRIVER;
    hdr.capability_transfer = 0;
    netmgr_ipc_handle_request(&hdr, &req, &res);
    assert(res.status == NETMGR_STATUS_ERR_PERM);

    // Test 12: Invalid OP without capability -> should fail with INVAL, not PERM (since it falls to default)
    req.opcode = NETMGR_OP_INVALID;
    hdr.capability_transfer = 0;
    netmgr_ipc_handle_request(&hdr, &req, &res);
    assert(res.status == NETMGR_STATUS_ERR_INVAL);

    return 0;
}
