#include <bharat/uapi/ipc/contract.h>
#include <bharat/uapi/ipc/status.h>
#include <bharat/network/netmgr_ipc.h>
#include <ipc_dispatch.h>
#include <ipc_contract.h>
#include <ipc_auth.h>
#include <service_manifest.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

// --- Stubs to satisfy linkage ---
void netmgr_iface_table_init() {}
void netmgr_addr_table_init() {}
void netmgr_route_table_init() {}
void netmgr_neighbor_cache_init() {}
void netmgr_driver_policy_init() {}
void netmgr_driver_health_init() {}

int netmgr_iface_create(const char* name, const uint8_t* mac, uint32_t mtu, net_if_id_t* new_id) { *new_id = 1; return NETMGR_STATUS_OK; }
void netmgr_driver_health_register(net_if_id_t id) {}
int netmgr_neighbor_flush(net_if_id_t id) { return NETMGR_STATUS_OK; }
void netmgr_driver_health_unregister(net_if_id_t id) {}
int netmgr_iface_delete(net_if_id_t id) { return NETMGR_STATUS_OK; }
int netmgr_iface_set_admin_state(net_if_id_t id, bool up) { return NETMGR_STATUS_OK; }
void* netmgr_iface_get(net_if_id_t id) { return NULL; }
int netmgr_addr_add(net_if_id_t id, net_af_t af, const uint8_t* addr, uint8_t len) { return NETMGR_STATUS_OK; }
int netmgr_addr_remove(net_if_id_t id, net_af_t af, const uint8_t* addr, uint8_t len) { return NETMGR_STATUS_OK; }
int netmgr_route_add(net_if_id_t id, net_af_t af, const uint8_t* dest, const uint8_t* mask, const uint8_t* gw, uint32_t metric) { return NETMGR_STATUS_OK; }
int netmgr_route_remove(net_af_t af, const uint8_t* dest, const uint8_t* mask) { return NETMGR_STATUS_OK; }
int netmgr_driver_health_request_restart(net_if_id_t id) { return NETMGR_STATUS_OK; }
bool bharat_cap_is_valid(bharat_cap_handle_t cap) { return cap != BHARAT_CAP_INVALID_HANDLE; }
// --------------------------------

void test_netmgr_valid_dispatch() {
    bharat_ipc_contract_header_t hdr;
    memset(&hdr, 0, sizeof(hdr));
    hdr.interface_version = 1;
    hdr.opcode = NETMGR_OP_CREATE_IFACE;
    hdr.payload_size = sizeof(struct netmgr_req_create_iface);
    // mock valid token (any non-zero is valid for now)
    hdr.capability_transfer = 1;

    netmgr_ipc_req_t req;
    memset(&req, 0, sizeof(req));
    req.opcode = NETMGR_OP_CREATE_IFACE;
    strncpy(req.u.create_iface.name, "eth0", sizeof(req.u.create_iface.name));

    netmgr_ipc_res_t res;

    netmgr_ipc_handle_request(&hdr, &req, &res);

    printf("test_netmgr_valid_dispatch run. Status: %d\n", res.status);
    assert(res.status == NETMGR_STATUS_OK);
    printf("test_netmgr_valid_dispatch passed\n");
}

void test_netmgr_invalid_version() {
    bharat_ipc_contract_header_t hdr;
    memset(&hdr, 0, sizeof(hdr));
    hdr.interface_version = 99; // invalid
    hdr.opcode = NETMGR_OP_CREATE_IFACE;

    netmgr_ipc_req_t req;
    memset(&req, 0, sizeof(req));
    req.opcode = NETMGR_OP_CREATE_IFACE;

    netmgr_ipc_res_t res;

    netmgr_ipc_handle_request(&hdr, &req, &res);

    // Version mismatch should be explicit
    assert(res.status == BHARAT_IPC_STATUS_ERR_VERSION);
    printf("test_netmgr_invalid_version passed\n");
}

void test_netmgr_permission_denied() {
    bharat_ipc_contract_header_t hdr;
    memset(&hdr, 0, sizeof(hdr));
    hdr.interface_version = 1;
    hdr.opcode = NETMGR_OP_CREATE_IFACE;
    hdr.capability_transfer = 0; // INVALID token
    hdr.payload_size = sizeof(struct netmgr_req_create_iface);

    netmgr_ipc_req_t req;
    memset(&req, 0, sizeof(req));
    req.opcode = NETMGR_OP_CREATE_IFACE;

    netmgr_ipc_res_t res;

    netmgr_ipc_handle_request(&hdr, &req, &res);

    // Should fail auth
    assert(res.status == NETMGR_STATUS_ERR_PERM);
    printf("test_netmgr_permission_denied passed\n");
}

void test_netmgr_invalid_payload_size() {
    bharat_ipc_contract_header_t hdr;
    memset(&hdr, 0, sizeof(hdr));
    hdr.interface_version = 1;
    hdr.opcode = NETMGR_OP_CREATE_IFACE;
    hdr.capability_transfer = 1;
    hdr.payload_size = 1; // Explicitly too small

    netmgr_ipc_req_t req;
    memset(&req, 0, sizeof(req));
    req.opcode = NETMGR_OP_CREATE_IFACE;

    netmgr_ipc_res_t res;

    netmgr_ipc_handle_request(&hdr, &req, &res);

    // Should fail dispatch validation
    assert(res.status == NETMGR_STATUS_ERR_INVAL);
    printf("test_netmgr_invalid_payload_size passed\n");
}

void test_netmgr_manifest() {
    assert(strcmp(netmgr_service_manifest.service_name, "netmgr") == 0);
    assert(netmgr_service_manifest.interface_version == 1);
    assert(netmgr_service_manifest.operation_count > 0);
    printf("test_netmgr_manifest passed\n");
}

int main() {
    netmgr_ipc_dispatch_init();
    test_netmgr_valid_dispatch();
    test_netmgr_invalid_version();
    test_netmgr_permission_denied();
    test_netmgr_invalid_payload_size();
    test_netmgr_manifest();
    return 0;
}