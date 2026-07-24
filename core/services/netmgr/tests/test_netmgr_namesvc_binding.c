#include <bharat/namesvc/client.h>
#include <bharat/service/service_runtime.h>
#include <bharat/uapi/services/service_ids.h>
#include <bharat/uapi/services/bootstrap.h>
#include <bharat/uapi/namesvc/contract.h>
#include <bharat/network/netmgr_ipc.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

// Mock IPC transport for host testing
static bharat_ipc_endpoint_t last_send_endpoint = 0;
static namesvc_ipc_req_t last_namesvc_req;
static namesvc_ipc_res_t next_namesvc_res;
static bharat_ipc_msg_header_t next_res_hdr;
static int namesvc_call_count = 0;

int32_t bharat_ipc_call(bharat_ipc_endpoint_t endpoint, const bharat_ipc_msg_header_t *req_header, const void *req_payload,
                        bharat_ipc_msg_header_t *rep_header, void *rep_payload_buf, uint32_t rep_max_size) {
    last_send_endpoint = endpoint;
    namesvc_call_count++;

    if (endpoint == BHARAT_BOOTSTRAP_NAMESVC_ENDPOINT) {
        memcpy(&last_namesvc_req, req_payload, sizeof(namesvc_ipc_req_t));
        memcpy(rep_payload_buf, &next_namesvc_res, sizeof(namesvc_ipc_res_t));
        *rep_header = next_res_hdr;
        rep_header->status = BHARAT_IPC_STATUS_OK;
        return BHARAT_IPC_STATUS_OK;
    }
    return -1;
}

int32_t bharat_ipc_send(bharat_ipc_endpoint_t endpoint, const bharat_ipc_msg_header_t *header, const void *payload) {
    (void)endpoint; (void)header; (void)payload;
    return BHARAT_IPC_STATUS_OK;
}

int32_t bharat_ipc_recv(bharat_ipc_endpoint_t endpoint, bharat_ipc_msg_header_t *header, void *payload_buf, uint32_t max_size) {
    (void)endpoint; (void)header; (void)payload_buf; (void)max_size;
    return BHARAT_IPC_STATUS_OK;
}

bharat_status_t bh_service_handle_msg(bh_service_ctx_t *ctx, const bh_msg_t *msg) {
    (void)ctx; (void)msg;
    return BHARAT_STATUS_OK;
}

// Mock syscall
long bh_syscall(long sysno, long arg1, long arg2, long arg3, long arg4, long arg5, long arg6) {
    (void)sysno; (void)arg1; (void)arg2; (void)arg3; (void)arg4; (void)arg5; (void)arg6;
    return 0;
}

// Host-test version of ipc_endpoint_create defined in ipc_user.h
long ipc_endpoint_create(uint32_t* out_send_cap, uint32_t* out_recv_cap) {
    *out_send_cap = 0x100;
    *out_recv_cap = 0x101;
    return 0;
}

int main(void) {
    printf("Starting Netmgr/Namesvc Binding Host Tests...\n");

    // Test 1: namesvc_register
    printf("Test 1: namesvc_register\n");
    memset(&next_namesvc_res, 0, sizeof(next_namesvc_res));
    next_namesvc_res.status = NAMESVC_STATUS_OK;

    int ret = namesvc_register("netmgr", BHARAT_SERVICE_NETMGR, 0x101, 1, 0);
    assert(ret == NAMESVC_STATUS_OK);
    assert(last_send_endpoint == BHARAT_BOOTSTRAP_NAMESVC_ENDPOINT);
    assert(last_namesvc_req.opcode == BHARAT_NAMESVC_OP_REGISTER);
    assert(strcmp(last_namesvc_req.u.reg.service_name, "netmgr") == 0);
    assert(last_namesvc_req.u.reg.service_id == BHARAT_SERVICE_NETMGR);
    assert(last_namesvc_req.u.reg.interface_version == 1);

    // Test 2: namesvc_lookup
    printf("Test 2: namesvc_lookup\n");
    memset(&next_namesvc_res, 0, sizeof(next_namesvc_res));
    memset(&next_res_hdr, 0, sizeof(next_res_hdr));
    next_namesvc_res.status = NAMESVC_STATUS_OK;
    next_namesvc_res.u.lookup_res.service_id = BHARAT_SERVICE_NETMGR;
    next_namesvc_res.u.lookup_res.interface_version = 1;
    next_res_hdr.capability_transfer = 0x101; // Should come from header, not payload

    bharat_service_id_t svc_id;
    bharat_ipc_endpoint_t ep;
    uint32_t ver;
    ret = namesvc_lookup("netmgr", &svc_id, &ep, &ver);
    assert(ret == NAMESVC_STATUS_OK);
    assert(svc_id == BHARAT_SERVICE_NETMGR);
    assert(ep == 0x101);
    assert(ver == 1);

    // Test 3: service_runtime_create_endpoint
    printf("Test 3: service_runtime_create_endpoint\n");
    ep = service_runtime_create_endpoint(BHARAT_SERVICE_NETMGR, 0);
    assert(ep == 0);

    printf("Netmgr/Namesvc Binding Host Tests Passed!\n");
    return 0;
}
