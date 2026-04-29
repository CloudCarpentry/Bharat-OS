#include <bharat/service/service_runtime.h>
#include <bharat/ipc/ipc.h>
#include <stddef.h>
#include <string.h>

// Well-known endpoint for service manager in this profile
#define SYSTEM_SERVICEMGR_ENDPOINT 0x3000

int bh_service_main(const bh_service_start_info_t *info) {
    if (!info) return BHARAT_STATUS_ERR_INTERNAL;

    bh_service_ctx_t ctx = {
        .service_id = info->service_id,
        .incarnation_id = 1, // In a real system, this would be passed in or negotiated
        .endpoint = info->endpoint,
        .user_data = info->user_data,
        .should_exit = false
    };

    // Signal we are starting
    bh_service_send_heartbeat(&ctx, SM_STATE_STARTING);

    // Specific services should call bh_service_signal_ready() when truly ready.
    // For the generic main, we'll wait for the event loop to start.
    bh_service_signal_ready(&ctx);

    while (!ctx.should_exit) {
        bh_service_poll_once(&ctx);
    }

    bh_service_send_heartbeat(&ctx, SM_STATE_STOPPED);
    return BHARAT_STATUS_OK;
}

bharat_status_t bh_service_poll_once(bh_service_ctx_t *ctx) {
    bharat_ipc_msg_header_t header;
    uint8_t buf[1024];

    int32_t ret = bharat_ipc_recv(ctx->endpoint, &header, buf, sizeof(buf));
    if (ret < 0) {
        return BHARAT_STATUS_ERR_INTERNAL;
    }

    bh_msg_t msg = {
        .header = header,
        .payload = buf,
        .payload_size = header.payload_size
    };

    return bh_service_handle_msg(ctx, &msg);
}

bharat_status_t bh_service_request_shutdown(bh_service_ctx_t *ctx) {
    ctx->should_exit = true;
    return BHARAT_STATUS_OK;
}

bharat_status_t bh_service_signal_ready(bh_service_ctx_t *ctx) {
    sm_req_heartbeat_t req = {
        .service_id = ctx->service_id,
        .incarnation_id = ctx->incarnation_id,
        .timestamp_ticks = 0
    };

    bharat_ipc_msg_header_t header = {
        .service_id = SERVICEMGR_SERVICE_ID,
        .opcode = SM_OP_SIGNAL_READY,
        .payload_size = sizeof(sm_req_heartbeat_t)
    };

    bharat_ipc_send(SYSTEM_SERVICEMGR_ENDPOINT, &header, &req);
    return BHARAT_STATUS_OK;
}

bharat_status_t bh_service_send_heartbeat(bh_service_ctx_t *ctx, uint32_t health_state) {
    sm_req_heartbeat_t req = {
        .service_id = ctx->service_id,
        .incarnation_id = ctx->incarnation_id,
        .health_state = health_state,
        .timestamp_ticks = 0
    };

    bharat_ipc_msg_header_t header = {
        .service_id = SERVICEMGR_SERVICE_ID,
        .opcode = SM_OP_HEARTBEAT,
        .payload_size = sizeof(sm_req_heartbeat_t)
    };

    bharat_ipc_send(SYSTEM_SERVICEMGR_ENDPOINT, &header, &req);
    return BHARAT_STATUS_OK;
}
