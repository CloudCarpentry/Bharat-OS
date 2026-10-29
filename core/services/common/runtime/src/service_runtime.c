#include <bharat/service/service_runtime.h>
#include <bharat/ipc/ipc.h>
#include <bharat/uapi/services/bootstrap.h>
#include <ipc_user.h>
#include <stddef.h>
#include <bharat/uapi/init/bootstrap.h>
#include <string.h>
#include <bharat/namesvc/client.h>

extern const bharat_user_startup_t *bharat_runtime_get_startup(void);

static bharat_ipc_endpoint_t cached_servicemgr_ep = BHARAT_CAP_INVALID_HANDLE;

static bharat_ipc_endpoint_t get_servicemgr_endpoint(void) {
    if (cached_servicemgr_ep != BHARAT_CAP_INVALID_HANDLE) {
        return cached_servicemgr_ep;
    }
    bharat_service_id_t svc_id;
    bharat_ipc_endpoint_t ep = BHARAT_CAP_INVALID_HANDLE;
    uint32_t version;
    int ret = namesvc_lookup("bharat.servicemgr", &svc_id, &ep, &version);
    if (ret == NAMESVC_STATUS_OK && ep != BHARAT_CAP_INVALID_HANDLE) {
        cached_servicemgr_ep = ep;
    }
    return cached_servicemgr_ep;
}

int bh_service_main(const bh_service_start_info_t *info) {
    if (!info || !info->process_id || !info->boot_session_id || !info->incarnation_id)
        return BHARAT_STATUS_ERR_INTERNAL;

    bh_service_ctx_t ctx = {
        .service_id = info->service_id,
        .incarnation_id = info->incarnation_id,
        .process_id = info->process_id,
        .boot_session_id = info->boot_session_id,
        .endpoint = info->endpoint,
        .user_data = info->user_data,
        .should_exit = false
    };

    // Signal we are spawned
    if (bh_service_send_health_update(&ctx, SM_HEALTH_STATE_SPAWNED, 0, 0) != BHARAT_STATUS_OK)
        return BHARAT_STATUS_ERR_INTERNAL;

    while (!ctx.should_exit) {
        if (bh_service_poll_once(&ctx) != BHARAT_STATUS_OK) return BHARAT_STATUS_ERR_INTERNAL;
    }

    return bh_service_send_health_update(&ctx, SM_HEALTH_STATE_STOPPED, 0, 0);
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

bharat_status_t bh_service_send_health_update(bh_service_ctx_t *ctx, sm_health_state_t state, int32_t status_code, uint64_t deadline_ticks) {
    if (!ctx || !ctx->process_id || !ctx->boot_session_id || !ctx->incarnation_id ||
        ctx->heartbeat_sequence == UINT64_MAX || state > SM_HEALTH_STATE_STOPPED)
        return BHARAT_STATUS_ERR_INTERNAL;

    sm_req_health_update_t req = {
        .struct_size = sizeof(sm_req_health_update_t),
        .service_id = ctx->service_id,
        .process_id = ctx->process_id,
        .boot_session_id = ctx->boot_session_id,
        .incarnation_id = ctx->incarnation_id,
        .health_state = (uint32_t)state,
        .padding = 0,
        .timestamp_ticks = 0,
        .heartbeat_sequence = ctx->heartbeat_sequence + 1,
        .deadline_ticks = deadline_ticks,
        .status_code = status_code
    };

    bharat_ipc_msg_header_t header = {
        .header_version = BHARAT_IPC_HEADER_VERSION_V1,
        .service_id = SERVICEMGR_SERVICE_ID,
        .opcode = SM_OP_HEALTH_UPDATE,
        .payload_size = sizeof(sm_req_health_update_t)
    };

    bharat_ipc_endpoint_t ep = get_servicemgr_endpoint();
    if (ep == BHARAT_CAP_INVALID_HANDLE || bharat_ipc_send(ep, &header, &req) != BHARAT_IPC_STATUS_OK)
        return BHARAT_STATUS_ERR_INTERNAL;
    ctx->heartbeat_sequence = req.heartbeat_sequence;
    return BHARAT_STATUS_OK;
}

bharat_ipc_endpoint_t service_runtime_create_endpoint(bharat_service_id_t service_id, uint32_t flags) {
    (void)service_id;
    (void)flags;
    uint32_t send_cap = 0;
    uint32_t recv_cap = 0;

    long ret = ipc_endpoint_create(&send_cap, &recv_cap);
    if (ret != 0) {
        return BHARAT_CAP_INVALID_HANDLE;
    }

    return (bharat_ipc_endpoint_t)recv_cap;
}

bharat_status_t service_runtime_bind_namesvc_bootstrap(bharat_ipc_endpoint_t endpoint) {
    const bharat_user_startup_t *startup = bharat_runtime_get_startup();
    if (startup && startup->bootstrap.service_receive_endpoint) {
        if (endpoint == startup->bootstrap.service_receive_endpoint) {
            return BHARAT_STATUS_OK;
        }
        return BHARAT_STATUS_ERR_NOT_FOUND;
    }
    // Phase A Transitional: In a real kernel, this might be a privileged syscall
    // or set up by the loader. For now, we assume the environment respects
    // BHARAT_BOOTSTRAP_NAMESVC_ENDPOINT.
    // If the endpoint handle is already what we expect, great.
    // If not, we might need a way to alias it, but for this slice we'll assume
    // the first created endpoint in namesvc *is* the bootstrap one or the
    // kernel/init has assigned it.

    if (endpoint == BHARAT_BOOTSTRAP_NAMESVC_ENDPOINT) {
        return BHARAT_STATUS_OK;
    }

    return BHARAT_STATUS_ERR_NOT_FOUND;
    // TODO(SERVICE-RUNTIME): Implement handle aliasing/rebinding if needed.
    return BHARAT_STATUS_OK;
}
