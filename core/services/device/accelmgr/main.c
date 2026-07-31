#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <kernel/status.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <bharat/uapi/services/service_ids.h>
#include <bharat/uapi/device/bharat_device_accelmgr_v2_types.h>
#include <bharat/uapi/ipc/status.h>
#include <bharat/uapi/ipc/manifest.h>
#include <bharat/accel/accel.h>
#include <bharat/cap/cap_validate.h>
#include <bharat/cap/cap_authz.h>
#include <bharat/uapi/capability/rights.h>

#include <bharat/service/service_runtime.h>
#include <bharat/ipc/ipc.h>

#include "accelmgr_broker.h"

// -------------------------------------------------------------------
// Service Runtime Handle Callback (Production Pathway)
// -------------------------------------------------------------------

static void send_reply(bharat_cap_handle_t target, const bharat_ipc_msg_header_t *orig_hdr, bharat_status_t status, void *payload, uint32_t payload_size) {
    bharat_ipc_msg_header_t reply_hdr = *orig_hdr;
    reply_hdr.flags |= BHARAT_IPC_FLAG_REPLY;
    reply_hdr.status = status;
    reply_hdr.payload_size = payload_size;
    bharat_ipc_send(target, &reply_hdr, payload);
}

bharat_status_t bh_service_handle_msg(bh_service_ctx_t *ctx, const bh_msg_t *msg) {
    (void)ctx;

    // We handle the BHARAT_SERVICE_ACCELMGR_V2 endpoint
    if (msg->header.service_id != BHARAT_SERVICE_ACCELMGR_V2) {
        return BHARAT_STATUS_OK;
    }

    // Use message header's capability transfer as the authenticated client cap, fallback to reply endpoint
    uint64_t client_cap = msg->header.capability_transfer;
    if (client_cap == BHARAT_CAP_INVALID_HANDLE) {
        client_cap = msg->header.reply_endpoint;
    }

    // Authenticated caller principal ID is simulated or extracted via process context on a real system
    uint64_t client_pid = 101;

    uint32_t opcode = msg->header.opcode;
    uint32_t resp_size = 0;
    uint8_t resp_buf[1024];
    memset(resp_buf, 0, sizeof(resp_buf));

    // Dynamic IDL RPC Dispatch
    switch (opcode) {
        case 1: { // GetBrokerInfo
            resp_size = sizeof(bharat_device_accelmgr_v2_GetBrokerInfoResp_t);
            handle_GetBrokerInfo(msg->payload, (void*)resp_buf, client_cap, client_pid);
            break;
        }
        case 2: { // GetDeviceCount
            resp_size = sizeof(bharat_device_accelmgr_v2_GetDeviceCountResp_t);
            handle_GetDeviceCount(msg->payload, (void*)resp_buf, client_cap, client_pid);
            break;
        }
        case 3: { // GetDeviceInfo
            resp_size = sizeof(bharat_device_accelmgr_v2_GetDeviceInfoResp_t);
            handle_GetDeviceInfo(msg->payload, (void*)resp_buf, client_cap, client_pid);
            break;
        }
        case 4: { // RequestAdmission
            resp_size = sizeof(bharat_device_accelmgr_v2_RequestAdmissionResp_t);
            handle_RequestAdmission(msg->payload, (void*)resp_buf, client_cap, client_pid);
            break;
        }
        case 5: { // ReleaseContext
            resp_size = sizeof(bharat_device_accelmgr_v2_ReleaseContextResp_t);
            handle_ReleaseContext(msg->payload, (void*)resp_buf, client_cap, client_pid);
            break;
        }
        case 6: { // CreateQueue
            resp_size = sizeof(bharat_device_accelmgr_v2_CreateQueueResp_t);
            handle_CreateQueue(msg->payload, (void*)resp_buf, client_cap, client_pid);
            break;
        }
        case 7: { // DestroyQueue
            resp_size = sizeof(bharat_device_accelmgr_v2_DestroyQueueResp_t);
            handle_DestroyQueue(msg->payload, (void*)resp_buf, client_cap, client_pid);
            break;
        }
        case 8: { // RegisterBuffer
            resp_size = sizeof(bharat_device_accelmgr_v2_RegisterBufferResp_t);
            handle_RegisterBuffer(msg->payload, (void*)resp_buf, client_cap, client_pid);
            break;
        }
        case 9: { // UnregisterBuffer
            resp_size = sizeof(bharat_device_accelmgr_v2_UnregisterBufferResp_t);
            handle_UnregisterBuffer(msg->payload, (void*)resp_buf, client_cap, client_pid);
            break;
        }
        case 10: { // SubmitJob
            resp_size = sizeof(bharat_device_accelmgr_v2_SubmitJobResp_t);
            handle_SubmitJob(msg->payload, (void*)resp_buf, client_cap, client_pid);
            break;
        }
        case 11: { // CancelJob
            resp_size = sizeof(bharat_device_accelmgr_v2_CancelJobResp_t);
            handle_CancelJob(msg->payload, (void*)resp_buf, client_cap, client_pid);
            break;
        }
        case 12: { // QueryJob
            resp_size = sizeof(bharat_device_accelmgr_v2_QueryJobResp_t);
            handle_QueryJob(msg->payload, (void*)resp_buf, client_cap, client_pid);
            break;
        }
        case 13: { // QueryFence
            resp_size = sizeof(bharat_device_accelmgr_v2_QueryFenceResp_t);
            handle_QueryFence(msg->payload, (void*)resp_buf, client_cap, client_pid);
            break;
        }
        case 14: { // QueryHealth
            resp_size = sizeof(bharat_device_accelmgr_v2_QueryHealthResp_t);
            handle_QueryHealth(msg->payload, (void*)resp_buf, client_cap, client_pid);
            break;
        }
        case 15: { // SetThermalConstraint
            resp_size = sizeof(bharat_device_accelmgr_v2_SetThermalConstraintResp_t);
            handle_SetThermalConstraint(msg->payload, (void*)resp_buf, client_cap, client_pid);
            break;
        }
        case 16: { // SetSafeMode
            resp_size = sizeof(bharat_device_accelmgr_v2_SetSafeModeResp_t);
            handle_SetSafeMode(msg->payload, (void*)resp_buf, client_cap, client_pid);
            break;
        }
        case 17: { // GetTelemetrySnapshot
            resp_size = sizeof(bharat_device_accelmgr_v2_GetTelemetrySnapshotResp_t);
            handle_GetTelemetrySnapshot(msg->payload, (void*)resp_buf, client_cap, client_pid);
            break;
        }
        default:
            send_reply(msg->header.reply_endpoint, &msg->header, BHARAT_IPC_STATUS_ERR_OPCODE, NULL, 0);
            return BHARAT_STATUS_OK;
    }

    send_reply(msg->header.reply_endpoint, &msg->header, BHARAT_STATUS_OK, resp_buf, resp_size);
    return BHARAT_STATUS_OK;
}

// Minimal main function loop
#ifndef ACCELMGR_NO_MAIN
int main(int argc, char **argv) {
    (void)argc; (void)argv;
    init_accelmgr();
    printf("accelmgr: Initializing hardware accelerator abstraction service V2...\n");

    // Real service start and registration loop!
    bh_service_start_info_t info = {
        .service_id = BHARAT_SERVICE_ACCELMGR_V2,
        .service_name = "accelmgr",
        .endpoint = BHARAT_CAP_INVALID_HANDLE
    };
    return bh_service_main(&info);
}
#endif
