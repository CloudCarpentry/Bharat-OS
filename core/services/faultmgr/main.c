
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <bharat/service/service_runtime.h>
#include <bharat/namesvc/client.h>
#include <bharat/uapi/services/service_ids.h>
#include <bharat/uapi/faultmgr/contract.h>
#include <bharat/ipc/ipc.h>
#include <bharat/bh_native.h>
#include <bharat/uapi/time/time.h>
#include "state.h"

/**
 * @file main.c
 * @brief faultmgr - Distributed crash containment and fault handling.
 */

void init_faultmgr(void) {
    faultmgr_state_init();
}

bharat_status_t bh_service_handle_msg(bh_service_ctx_t *ctx, const bh_msg_t *msg) {
    if (!ctx || !msg) {
        return BHARAT_STATUS_ERR_INVALID_ARG;
    }

    if (msg->header.opcode != FAULTMGR_OP_REPORT_FAULT) {
        return BHARAT_STATUS_ERR_OPCODE;
    }

    if (msg->payload_size < sizeof(faultmgr_req_report_fault_t)) {
        return BHARAT_STATUS_ERR_LENGTH;
    }

    const faultmgr_req_report_fault_t *req = (const faultmgr_req_report_fault_t *)msg->payload;

    bh_time_t current_time = 0;
    bh_time_get(BH_CLOCK_MONOTONIC, &current_time);

    faultmgr_recovery_action_t action = faultmgr_process_event(req, current_time);

    if (msg->header.flags & BHARAT_IPC_FLAG_REPLY) {
        faultmgr_resp_report_fault_t resp = {
            .status = 0,
            .action = action
        };

        bharat_ipc_msg_header_t reply_hdr = {
            .service_id = BHARAT_SERVICE_FAULTMGR,
            .opcode = FAULTMGR_OP_REPORT_FAULT,
            .payload_size = sizeof(faultmgr_resp_report_fault_t),
            .flags = 0
        };

        if (msg->header.reply_endpoint != BHARAT_CAP_INVALID_HANDLE) {
            bharat_ipc_send((bharat_ipc_endpoint_t)msg->header.reply_endpoint, &reply_hdr, &resp);
        }
    }

    return BHARAT_STATUS_OK;
}

#ifndef FAULTMGR_UNIT_TEST
int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    init_faultmgr();

    bharat_ipc_endpoint_t ep = service_runtime_create_endpoint(BHARAT_SERVICE_FAULTMGR, 0);
    if (ep == BHARAT_CAP_INVALID_HANDLE) {
        return -1;
    }

    int ret = namesvc_register("bharat.faultmgr", BHARAT_SERVICE_FAULTMGR, ep, 1, 0);
    if (ret != NAMESVC_STATUS_OK) {
        return -1;
    }

    bh_service_start_info_t info = {
        .service_id = BHARAT_SERVICE_FAULTMGR,
        .service_name = "bharat.faultmgr",
        .endpoint = ep,
        .user_data = NULL
    };

    return bh_service_main(&info);
}
#endif
