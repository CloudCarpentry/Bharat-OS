#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <bharat/service/service_runtime.h>
#include <bharat/uapi/devmgr/contract.h>
#include <bharat/ipc/ipc.h>
#include <bharat/bh_native.h>
#include "devmgr_core.h"

#define MAX_DEVICES 64

typedef enum {
    STATE_DISCOVERED = 0,
    STATE_AVAILABLE,
    STATE_BINDING,
    STATE_BOUND,
    STATE_SUSPENDING,
    STATE_SUSPENDED,
    STATE_FAILED,
    STATE_REMOVING,
    STATE_REMOVED
} local_device_state_t;

typedef struct {
    uint32_t registry_id;
    uint32_t bus;
    uint32_t device_id;
    uint32_t vendor_id;
    uint32_t class_id;
    char name[32];
    char compatible[32];
    char bound_driver[32];
    local_device_state_t state;
    bool in_use;
} devmgr_device_t;

static devmgr_device_t g_devices[MAX_DEVICES];
static uint32_t g_next_registry_id = 1;

void devmgr_registry_init(void) {
    for (int i = 0; i < MAX_DEVICES; i++) {
        g_devices[i].in_use = false;
    }
}

static devmgr_device_t* find_device_by_identity(uint32_t bus, uint32_t device_id) {
    for (int i = 0; i < MAX_DEVICES; i++) {
        if (g_devices[i].in_use && g_devices[i].bus == bus && g_devices[i].device_id == device_id) {
            return &g_devices[i];
        }
    }
    return NULL;
}

static devmgr_device_t* find_device_by_registry_id(uint32_t id) {
    for (int i = 0; i < MAX_DEVICES; i++) {
        if (g_devices[i].in_use && g_devices[i].registry_id == id) {
            return &g_devices[i];
        }
    }
    return NULL;
}

static devmgr_device_t* allocate_device(void) {
    for (int i = 0; i < MAX_DEVICES; i++) {
        if (!g_devices[i].in_use) {
            g_devices[i].in_use = true;
            g_devices[i].registry_id = g_next_registry_id++;
            g_devices[i].state = STATE_DISCOVERED;
            g_devices[i].bound_driver[0] = '\0';
            return &g_devices[i];
        }
    }
    return NULL;
}

static bharat_status_t handle_hotplug(bh_service_ctx_t *ctx, const bh_msg_t *msg) {
    if (msg->payload_size < sizeof(devmgr_req_hotplug_t)) {
        return BHARAT_IPC_STATUS_ERR_LENGTH;
    }
    const devmgr_req_hotplug_t *req = (const devmgr_req_hotplug_t *)msg->payload;

    if (find_device_by_identity(req->bus, req->device_id)) {
        return BHARAT_IPC_STATUS_ERR_INVALID; // duplicate
    }

    devmgr_device_t *dev = allocate_device();
    if (!dev) {
        return BHARAT_IPC_STATUS_ERR_NOT_FOUND; // out of space
    }

    dev->bus = req->bus;
    dev->device_id = req->device_id;
    dev->vendor_id = req->vendor_id;
    dev->class_id = req->class_id;
    strncpy(dev->name, req->name, sizeof(dev->name) - 1);
    dev->name[sizeof(dev->name) - 1] = '\0';
    strncpy(dev->compatible, req->compatible, sizeof(dev->compatible) - 1);
    dev->compatible[sizeof(dev->compatible) - 1] = '\0';

    dev->state = STATE_AVAILABLE;

    if (msg->header.flags & BHARAT_IPC_FLAG_REPLY) {
        devmgr_resp_hotplug_t resp = {
            .status = BHARAT_STATUS_OK,
            .device_registry_id = dev->registry_id
        };
        bharat_ipc_msg_header_t hdr = {
            .opcode = DEVMGR_OP_HOTPLUG_DEVICE,
            .payload_size = sizeof(resp)
        };
        bharat_ipc_send(msg->header.reply_endpoint, &hdr, &resp);
    }

    return BHARAT_STATUS_OK;
}

static bharat_status_t handle_bind(bh_service_ctx_t *ctx, const bh_msg_t *msg) {
    if (msg->payload_size < sizeof(devmgr_req_bind_t)) {
        return BHARAT_IPC_STATUS_ERR_LENGTH;
    }
    const devmgr_req_bind_t *req = (const devmgr_req_bind_t *)msg->payload;

    if (msg->header.capability_transfer == 0) {
        // Needs authorization token
        return BHARAT_IPC_STATUS_ERR_PERM;
    }

    devmgr_device_t *dev = find_device_by_registry_id(req->device_registry_id);
    if (!dev) {
        return BHARAT_IPC_STATUS_ERR_NOT_FOUND;
    }

    if (dev->state != STATE_AVAILABLE && dev->state != STATE_FAILED) {
        return BHARAT_IPC_STATUS_ERR_INVALID;
    }

    dev->state = STATE_BINDING;

    // Simulate binding logic rollback on invalid driver name
    if (req->driver_name[0] == '\0') {
        dev->state = STATE_FAILED;
        return BHARAT_IPC_STATUS_ERR_INVALID;
    }

    strncpy(dev->bound_driver, req->driver_name, sizeof(dev->bound_driver) - 1);
    dev->bound_driver[sizeof(dev->bound_driver) - 1] = '\0';
    dev->state = STATE_BOUND;

    if (msg->header.flags & BHARAT_IPC_FLAG_REPLY) {
        int32_t status = BHARAT_STATUS_OK;
        bharat_ipc_msg_header_t hdr = {
            .opcode = DEVMGR_OP_BIND_DRIVER,
            .payload_size = sizeof(status)
        };
        bharat_ipc_send(msg->header.reply_endpoint, &hdr, &status);
    }

    return BHARAT_STATUS_OK;
}

static bharat_status_t handle_remove(bh_service_ctx_t *ctx, const bh_msg_t *msg) {
    if (msg->payload_size < sizeof(devmgr_req_remove_t)) {
        return BHARAT_IPC_STATUS_ERR_LENGTH;
    }
    const devmgr_req_remove_t *req = (const devmgr_req_remove_t *)msg->payload;

    devmgr_device_t *dev = find_device_by_registry_id(req->device_registry_id);
    if (!dev) {
        return BHARAT_IPC_STATUS_ERR_NOT_FOUND;
    }

    dev->state = STATE_REMOVING;
    dev->in_use = false;
    dev->state = STATE_REMOVED;

    if (msg->header.flags & BHARAT_IPC_FLAG_REPLY) {
        int32_t status = BHARAT_STATUS_OK;
        bharat_ipc_msg_header_t hdr = {
            .opcode = DEVMGR_OP_REMOVE_DEVICE,
            .payload_size = sizeof(status)
        };
        bharat_ipc_send(msg->header.reply_endpoint, &hdr, &status);
    }

    return BHARAT_STATUS_OK;
}

static bharat_status_t handle_unsupported_privileged_op(bh_service_ctx_t *ctx, const bh_msg_t *msg) {
    // Return explicit error for IOMMU, IRQ mapping if they are sent as custom unhandled opcodes
    return BHARAT_IPC_STATUS_ERR_UNSUPPORTED;
}

bharat_status_t devmgr_handle_message(bh_service_ctx_t *ctx, const bh_msg_t *msg) {
    if (!ctx || !msg) return BHARAT_IPC_STATUS_ERR_INVALID;

    switch (msg->header.opcode) {
        case DEVMGR_OP_HOTPLUG_DEVICE:
            return handle_hotplug(ctx, msg);
        case DEVMGR_OP_BIND_DRIVER:
            return handle_bind(ctx, msg);
        case DEVMGR_OP_REMOVE_DEVICE:
            return handle_remove(ctx, msg);
        default:
            return handle_unsupported_privileged_op(ctx, msg);
    }
}