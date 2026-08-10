#include "servicemgr.h"
#include <bharat/namesvc/client.h>
#include <bharat/uapi/process_manager/contract.h>
#include <bharat/uapi/ipc/status.h>
#include <stddef.h>

static bharat_ipc_endpoint_t pm_endpoint = BHARAT_CAP_INVALID_HANDLE;

static bharat_ipc_endpoint_t get_pm_endpoint(void) {
    if (pm_endpoint != BHARAT_CAP_INVALID_HANDLE) {
        return pm_endpoint;
    }

    bharat_service_id_t service_id;
    bharat_ipc_endpoint_t ep = BHARAT_CAP_INVALID_HANDLE;
    uint32_t version;

    int ret = namesvc_lookup("process_manager", &service_id, &ep, &version);
    if (ret == NAMESVC_STATUS_OK && ep != BHARAT_CAP_INVALID_HANDLE) {
        pm_endpoint = ep;
    }
    return pm_endpoint;
}

static int32_t pm_spawn(
    void *ctx,
    const bh_service_launch_spec_t *spec,
    bh_service_launch_result_t *out)
{
    (void)ctx;
    if (!spec || !out) return BHARAT_IPC_STATUS_ERR_INVALID;

    bharat_ipc_endpoint_t ep = get_pm_endpoint();
    if (ep == BHARAT_CAP_INVALID_HANDLE) {
        // Return explicit unsupported/unavailable error if cannot resolve real spawning path
        return BHARAT_IPC_STATUS_ERR_PERM;
    }

    // Call PM_OP_CREATE
    bharat_ipc_msg_header_t req_hdr = {
        .header_version = BHARAT_IPC_HEADER_VERSION_V1,
        .service_id = PROCESS_MANAGER_SERVICE_ID,
        .opcode = PM_OP_CREATE,
        .payload_size = sizeof(pm_req_create_t)
    };

    pm_req_create_t req_create = {
        .executable_id = spec->executable_id,
        .priority = spec->priority
    };

    bharat_ipc_msg_header_t rep_hdr = {0};
    pm_resp_create_t rep_create = {0};

    int32_t call_status = bharat_ipc_call_ex(ep, &req_hdr, &req_create, &rep_hdr, &rep_create, sizeof(rep_create), 1000);
    if (call_status != BHARAT_IPC_STATUS_OK || rep_create.status != BHARAT_IPC_STATUS_OK) {
        return BHARAT_IPC_STATUS_ERR_PERM;
    }

    // Call PM_OP_START
    req_hdr.opcode = PM_OP_START;
    req_hdr.payload_size = sizeof(pm_req_start_t);

    pm_req_start_t req_start = {
        .process_id = rep_create.process_id
    };

    pm_resp_start_t rep_start = {0};

    call_status = bharat_ipc_call_ex(ep, &req_hdr, &req_start, &rep_hdr, &rep_start, sizeof(rep_start), 1000);
    if (call_status != BHARAT_IPC_STATUS_OK || rep_start.status != BHARAT_IPC_STATUS_OK) {
        return BHARAT_IPC_STATUS_ERR_PERM;
    }

    out->process_id = rep_create.process_id;
    out->status = BHARAT_IPC_STATUS_OK;
    return BHARAT_IPC_STATUS_OK;
}

static int32_t pm_request_stop(
    void *ctx,
    uint32_t process_id)
{
    (void)ctx;
    bharat_ipc_endpoint_t ep = get_pm_endpoint();
    if (ep == BHARAT_CAP_INVALID_HANDLE) {
        return BHARAT_IPC_STATUS_ERR_PERM;
    }

    bharat_ipc_msg_header_t req_hdr = {
        .header_version = BHARAT_IPC_HEADER_VERSION_V1,
        .service_id = PROCESS_MANAGER_SERVICE_ID,
        .opcode = PM_OP_STOP,
        .payload_size = sizeof(pm_req_stop_t)
    };

    pm_req_stop_t req_stop = {
        .process_id = process_id
    };

    bharat_ipc_msg_header_t rep_hdr = {0};
    pm_resp_stop_t rep_stop = {0};

    int32_t call_status = bharat_ipc_call_ex(ep, &req_hdr, &req_stop, &rep_hdr, &rep_stop, sizeof(rep_stop), 1000);
    if (call_status != BHARAT_IPC_STATUS_OK) {
        return call_status;
    }
    return rep_stop.status;
}

static int32_t pm_query(
    void *ctx,
    uint32_t process_id,
    uint32_t *out_state)
{
    (void)ctx;
    if (!out_state) return BHARAT_IPC_STATUS_ERR_INVALID;

    bharat_ipc_endpoint_t ep = get_pm_endpoint();
    if (ep == BHARAT_CAP_INVALID_HANDLE) {
        return BHARAT_IPC_STATUS_ERR_PERM;
    }

    bharat_ipc_msg_header_t req_hdr = {
        .header_version = BHARAT_IPC_HEADER_VERSION_V1,
        .service_id = PROCESS_MANAGER_SERVICE_ID,
        .opcode = PM_OP_QUERY,
        .payload_size = sizeof(pm_req_query_t)
    };

    pm_req_query_t req_query = {
        .process_id = process_id
    };

    bharat_ipc_msg_header_t rep_hdr = {0};
    pm_resp_query_t rep_query = {0};

    int32_t call_status = bharat_ipc_call_ex(ep, &req_hdr, &req_query, &rep_hdr, &rep_query, sizeof(rep_query), 1000);
    if (call_status != BHARAT_IPC_STATUS_OK) {
        return call_status;
    }

    if (rep_query.status == BHARAT_IPC_STATUS_OK) {
        *out_state = rep_query.state;
    }
    return rep_query.status;
}

const bh_service_launcher_ops_t g_launcher_processmgr_ops = {
    .ctx = NULL,
    .spawn = pm_spawn,
    .request_stop = pm_request_stop,
    .query = pm_query
};
