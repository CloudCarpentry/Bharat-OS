#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <bharat/uapi/devmgr/contract.h>
#include <bharat/service/service_runtime.h>
#include <bharat/ipc/ipc.h>

// Forward declare the handler from main.c
extern bharat_status_t devmgr_handle_message(bh_service_ctx_t *ctx, const bh_msg_t *msg);

// Mocks
static bharat_ipc_msg_header_t g_last_reply_hdr;
static uint8_t g_last_reply_payload[1024];

int32_t bharat_ipc_send(bharat_ipc_endpoint_t endpoint, const bharat_ipc_msg_header_t *header, const void *payload) {
    g_last_reply_hdr = *header;
    if (payload && header->payload_size > 0) {
        memcpy(g_last_reply_payload, payload, header->payload_size);
    }
    return 0;
}

int main() {
    bh_service_ctx_t ctx = {
        .service_id = DEVMGR_SERVICE_ID,
        .endpoint = 1
    };

    printf("Test 1: Hotplug device\n");
    devmgr_req_hotplug_t hotplug_req = {
        .bus = 1,
        .device_id = 100,
        .vendor_id = 0x1234,
        .class_id = 0x02,
        .name = "test_dev",
        .compatible = "test,dev"
    };
    bharat_ipc_msg_header_t hotplug_hdr = {
        .opcode = DEVMGR_OP_HOTPLUG_DEVICE,
        .flags = BHARAT_IPC_FLAG_REPLY,
        .payload_size = sizeof(hotplug_req),
        .reply_endpoint = 2
    };
    bh_msg_t hotplug_msg = {
        .header = hotplug_hdr,
        .payload = &hotplug_req,
        .payload_size = sizeof(hotplug_req)
    };

    bharat_status_t status = devmgr_handle_message(&ctx, &hotplug_msg);
    assert(status == BHARAT_STATUS_OK);
    assert(g_last_reply_hdr.opcode == DEVMGR_OP_HOTPLUG_DEVICE);

    devmgr_resp_hotplug_t *resp = (devmgr_resp_hotplug_t *)g_last_reply_payload;
    assert(resp->status == BHARAT_STATUS_OK);
    uint32_t dev_id = resp->device_registry_id;
    assert(dev_id == 1);

    printf("Test 2: Duplicate hotplug device\n");
    status = devmgr_handle_message(&ctx, &hotplug_msg);
    assert(status == BHARAT_IPC_STATUS_ERR_INVALID);

    printf("Test 3: Bind driver\n");
    devmgr_req_bind_t bind_req = {
        .device_registry_id = dev_id,
        .driver_name = "test_driver"
    };
    bharat_ipc_msg_header_t bind_hdr = {
        .opcode = DEVMGR_OP_BIND_DRIVER,
        .flags = BHARAT_IPC_FLAG_REPLY,
        .capability_transfer = 1, // Require capability transfer token
        .payload_size = sizeof(bind_req),
        .reply_endpoint = 2
    };
    bh_msg_t bind_msg = {
        .header = bind_hdr,
        .payload = &bind_req,
        .payload_size = sizeof(bind_req)
    };

    status = devmgr_handle_message(&ctx, &bind_msg);
    assert(status == BHARAT_STATUS_OK);
    int32_t *bind_status = (int32_t *)g_last_reply_payload;
    assert(*bind_status == BHARAT_STATUS_OK);

    printf("Test 4: Bind driver without capability token\n");
    bharat_ipc_msg_header_t bind_hdr_no_cap = bind_hdr;
    bind_hdr_no_cap.capability_transfer = 0;
    bh_msg_t bind_msg_no_cap = {
        .header = bind_hdr_no_cap,
        .payload = &bind_req,
        .payload_size = sizeof(bind_req)
    };
    status = devmgr_handle_message(&ctx, &bind_msg_no_cap);
    assert(status == BHARAT_IPC_STATUS_ERR_PERM);

    printf("Test 5: Bind rollback on invalid driver name\n");
    devmgr_req_bind_t bad_bind_req = {
        .device_registry_id = dev_id,
        .driver_name = ""
    };
    bh_msg_t bad_bind_msg = {
        .header = bind_hdr,
        .payload = &bad_bind_req,
        .payload_size = sizeof(bad_bind_req)
    };
    status = devmgr_handle_message(&ctx, &bad_bind_msg);
    assert(status == BHARAT_IPC_STATUS_ERR_INVALID);

    printf("Test 6: Remove device\n");
    devmgr_req_remove_t remove_req = {
        .device_registry_id = dev_id
    };
    bharat_ipc_msg_header_t remove_hdr = {
        .opcode = DEVMGR_OP_REMOVE_DEVICE,
        .flags = BHARAT_IPC_FLAG_REPLY,
        .payload_size = sizeof(remove_req),
        .reply_endpoint = 2
    };
    bh_msg_t remove_msg = {
        .header = remove_hdr,
        .payload = &remove_req,
        .payload_size = sizeof(remove_req)
    };
    status = devmgr_handle_message(&ctx, &remove_msg);
    assert(status == BHARAT_STATUS_OK);
    int32_t *remove_status = (int32_t *)g_last_reply_payload;
    assert(*remove_status == BHARAT_STATUS_OK);

    printf("Test 7: Unsupported privileged operation\n");
    bharat_ipc_msg_header_t unsupported_hdr = {
        .opcode = 9999, // Unhandled opcode
        .payload_size = 0,
    };
    bh_msg_t unsupported_msg = {
        .header = unsupported_hdr,
        .payload = NULL,
        .payload_size = 0
    };
    status = devmgr_handle_message(&ctx, &unsupported_msg);
    assert(status == BHARAT_IPC_STATUS_ERR_UNSUPPORTED);

    printf("All devmgr host tests passed.\n");
    return 0;
}