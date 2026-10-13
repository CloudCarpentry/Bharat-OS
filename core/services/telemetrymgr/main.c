#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "services/telemetrymgr/thermal_policy.h"
#include <bharat/service/service_runtime.h>
#include <bharat/runtime/runtime.h>
#include <bharat/uapi/telemetrymgr/contract.h>
#include "registry.h"

/**
 * @file main.c
 * @brief telemetrymgr - System-wide metrics and monitoring.
 */

typedef struct {
    metrics_registry_t registry;

    // Kept for tests, but we don't hardcode them on startup
    thermal_zone_reading_t zones[4];
    uint32_t zone_count;
    thermal_policy_state_t latest;
} telemetrymgr_state_t;

static telemetrymgr_state_t g_state;

void init_telemetrymgr(void) {
    registry_init(&g_state.registry);
    g_state.zone_count = 0;
    // Don't invent temperatures if none arrived yet
}

static bharat_status_t handle_register_metric(bh_service_ctx_t *ctx, const bh_msg_t *msg) {
    (void)ctx;

    // Authorization: Only privileged clients can register metrics.
    // Assuming capability token is required (non-zero) or specific source
    if (msg->header.capability_transfer == 0) {
        return BHARAT_STATUS_ERR_PERMISSION;
    }

    if (msg->payload_size < sizeof(telemetrymgr_req_register_metric_t)) {
        g_state.registry.invalid_message_count++;
        return BHARAT_STATUS_ERR_LENGTH;
    }
    const telemetrymgr_req_register_metric_t *req = (const telemetrymgr_req_register_metric_t *)msg->payload;
    telemetrymgr_resp_register_metric_t resp = {0};

    uint32_t cid;
    resp.status = registry_register_metric(&g_state.registry, &req->desc, &cid);
    if (resp.status == BHARAT_STATUS_OK) {
        resp.counter_id = cid;
    }

    if (msg->header.flags & BHARAT_IPC_FLAG_REPLY) {
        bharat_ipc_msg_header_t hdr = {
            .opcode = TELEMETRYMGR_OP_REGISTER_METRIC,
            .payload_size = sizeof(resp)
        };
        bharat_ipc_send(msg->header.reply_endpoint, &hdr, &resp);
    }

    return BHARAT_STATUS_OK;
}

static bharat_status_t handle_update_counter(bh_service_ctx_t *ctx, const bh_msg_t *msg) {
    (void)ctx;

    // Authorization: Only privileged clients can update metrics.
    if (msg->header.capability_transfer == 0) {
        return BHARAT_STATUS_ERR_PERMISSION;
    }

    if (msg->payload_size < sizeof(telemetrymgr_req_update_metric_t)) {
        g_state.registry.invalid_message_count++;
        return BHARAT_STATUS_ERR_LENGTH;
    }
    const telemetrymgr_req_update_metric_t *req = (const telemetrymgr_req_update_metric_t *)msg->payload;
    telemetrymgr_resp_update_metric_t resp = {0};

    resp.status = registry_update_counter(&g_state.registry, req->counter_id, req->value, req->sequence);

    if (msg->header.flags & BHARAT_IPC_FLAG_REPLY) {
        bharat_ipc_msg_header_t hdr = {
            .opcode = TELEMETRYMGR_OP_UPDATE_COUNTER,
            .payload_size = sizeof(resp)
        };
        bharat_ipc_send(msg->header.reply_endpoint, &hdr, &resp);
    }
    return BHARAT_STATUS_OK;
}

static bharat_status_t handle_update_gauge(bh_service_ctx_t *ctx, const bh_msg_t *msg) {
    (void)ctx;

    // Authorization: Only privileged clients can update metrics.
    if (msg->header.capability_transfer == 0) {
        return BHARAT_STATUS_ERR_PERMISSION;
    }

    if (msg->payload_size < sizeof(telemetrymgr_req_update_metric_t)) {
        g_state.registry.invalid_message_count++;
        return BHARAT_STATUS_ERR_LENGTH;
    }
    const telemetrymgr_req_update_metric_t *req = (const telemetrymgr_req_update_metric_t *)msg->payload;
    telemetrymgr_resp_update_metric_t resp = {0};

    resp.status = registry_update_gauge(&g_state.registry, req->counter_id, req->value, req->sequence);

    // Thermal integration: Check if it's a thermal metric update
    bharat_counter_snapshot_t snap;
    if (registry_query_metric(&g_state.registry, req->counter_id, &snap) == BHARAT_STATUS_OK) {
        // Find descriptor to check category/name (simplified heuristic for thermal zones)
        for (uint32_t i = 0; i < MAX_METRICS; i++) {
            if (g_state.registry.metrics[i].active && g_state.registry.metrics[i].desc.counter_id == req->counter_id) {
                if (strncmp(g_state.registry.metrics[i].desc.unit, "celsius", 7) == 0 ||
                    strncmp(g_state.registry.metrics[i].desc.unit, "mc", 2) == 0) {
                    // Inject into thermal policy
                    // Determine which zone this source_id maps to
                    uint32_t zone_idx = g_state.zone_count;
                    for (uint32_t z = 0; z < g_state.zone_count; z++) {
                        // Assuming one thermal metric per source for simplicity
                        // In reality, this would map the specific counter_id to a zone
                        // For the current naive mapping, we update the existing zone or append
                        // We will map based on position up to 4 zones
                        if (z == (req->counter_id % 4)) {
                            zone_idx = z;
                            break;
                        }
                    }

                    if (zone_idx < 4) {
                        g_state.zones[zone_idx].temp_mc = req->value;
                        g_state.zones[zone_idx].passive_trip_mc = 70000;
                        g_state.zones[zone_idx].hot_trip_mc = 85000;
                        g_state.zones[zone_idx].critical_trip_mc = 95000;
                        if (zone_idx == g_state.zone_count) {
                            g_state.zone_count++;
                        }
                    }
                    g_state.latest = thermal_policy_apply(g_state.zones, g_state.zone_count);
                }
                break;
            }
        }
    }

    if (msg->header.flags & BHARAT_IPC_FLAG_REPLY) {
        bharat_ipc_msg_header_t hdr = {
            .opcode = TELEMETRYMGR_OP_UPDATE_GAUGE,
            .payload_size = sizeof(resp)
        };
        bharat_ipc_send(msg->header.reply_endpoint, &hdr, &resp);
    }
    return BHARAT_STATUS_OK;
}

static bharat_status_t handle_query_metric(bh_service_ctx_t *ctx, const bh_msg_t *msg) {
    (void)ctx;

    if (msg->payload_size < sizeof(telemetrymgr_req_query_metric_t)) {
        g_state.registry.invalid_message_count++;
        return BHARAT_STATUS_ERR_LENGTH;
    }
    const telemetrymgr_req_query_metric_t *req = (const telemetrymgr_req_query_metric_t *)msg->payload;
    telemetrymgr_resp_query_metric_t resp = {0};

    resp.status = registry_query_metric(&g_state.registry, req->counter_id, &resp.snapshot);

    if (msg->header.flags & BHARAT_IPC_FLAG_REPLY) {
        bharat_ipc_msg_header_t hdr = {
            .opcode = TELEMETRYMGR_OP_QUERY_METRIC,
            .payload_size = sizeof(resp)
        };
        bharat_ipc_send(msg->header.reply_endpoint, &hdr, &resp);
    }
    return BHARAT_STATUS_OK;
}

static bharat_status_t handle_request_snapshot(bh_service_ctx_t *ctx, const bh_msg_t *msg) {
    (void)ctx;

    if (msg->payload_size < sizeof(telemetrymgr_req_request_snapshot_t)) {
        g_state.registry.invalid_message_count++;
        return BHARAT_STATUS_ERR_LENGTH;
    }
    const telemetrymgr_req_request_snapshot_t *req = (const telemetrymgr_req_request_snapshot_t *)msg->payload;

    uint8_t resp_buf[sizeof(telemetrymgr_resp_request_snapshot_t) + sizeof(bharat_counter_snapshot_t) * 16];
    telemetrymgr_resp_request_snapshot_t *resp = (telemetrymgr_resp_request_snapshot_t *)resp_buf;
    bharat_counter_snapshot_t *snaps = (bharat_counter_snapshot_t *)(resp + 1);

    resp->status = BHARAT_STATUS_OK;
    resp->total_count = g_state.registry.active_count;
    resp->overflow_count = g_state.registry.overflow_count;
    resp->invalid_message_count = g_state.registry.invalid_message_count;

    uint32_t current_idx = 0;
    uint32_t copied = 0;
    uint32_t max_copy = req->max_count;
    if (max_copy > 16) max_copy = 16;

    for (uint32_t i = 0; i < MAX_METRICS; i++) {
        if (g_state.registry.metrics[i].active) {
            if (current_idx >= req->start_index && copied < max_copy) {
                snaps[copied] = g_state.registry.metrics[i].snapshot;
                copied++;
            }
            current_idx++;
        }
    }
    resp->count = copied;

    if (msg->header.flags & BHARAT_IPC_FLAG_REPLY) {
        bharat_ipc_msg_header_t hdr = {
            .opcode = TELEMETRYMGR_OP_REQUEST_SNAPSHOT,
            .payload_size = sizeof(telemetrymgr_resp_request_snapshot_t) + sizeof(bharat_counter_snapshot_t) * copied
        };
        bharat_ipc_send(msg->header.reply_endpoint, &hdr, resp_buf);
    }
    return BHARAT_STATUS_OK;
}

static bharat_status_t handle_reset_metric(bh_service_ctx_t *ctx, const bh_msg_t *msg) {
    (void)ctx;

    // Authorization: Only privileged clients can reset metrics.
    if (msg->header.capability_transfer == 0) {
        return BHARAT_STATUS_ERR_PERMISSION;
    }

    if (msg->payload_size < sizeof(telemetrymgr_req_reset_metric_t)) {
        g_state.registry.invalid_message_count++;
        return BHARAT_STATUS_ERR_LENGTH;
    }
    const telemetrymgr_req_reset_metric_t *req = (const telemetrymgr_req_reset_metric_t *)msg->payload;
    telemetrymgr_resp_reset_metric_t resp = {0};

    resp.status = registry_reset_metric(&g_state.registry, req->counter_id);

    if (msg->header.flags & BHARAT_IPC_FLAG_REPLY) {
        bharat_ipc_msg_header_t hdr = {
            .opcode = TELEMETRYMGR_OP_RESET_METRIC,
            .payload_size = sizeof(resp)
        };
        bharat_ipc_send(msg->header.reply_endpoint, &hdr, &resp);
    }
    return BHARAT_STATUS_OK;
}


bharat_status_t bh_service_handle_msg(bh_service_ctx_t *ctx, const bh_msg_t *msg) {
    bharat_runtime_log("telemetrymgr: received message (opcode: %d)", msg->header.opcode);
    switch (msg->header.opcode) {
        case TELEMETRYMGR_OP_REGISTER_METRIC:
            return handle_register_metric(ctx, msg);
        case TELEMETRYMGR_OP_UPDATE_COUNTER:
            return handle_update_counter(ctx, msg);
        case TELEMETRYMGR_OP_UPDATE_GAUGE:
            return handle_update_gauge(ctx, msg);
        case TELEMETRYMGR_OP_QUERY_METRIC:
            return handle_query_metric(ctx, msg);
        case TELEMETRYMGR_OP_REQUEST_SNAPSHOT:
            return handle_request_snapshot(ctx, msg);
        case TELEMETRYMGR_OP_RESET_METRIC:
            return handle_reset_metric(ctx, msg);
        case TELEMETRYMGR_OP_SHUTDOWN:
            bh_service_request_shutdown(ctx);
            return BHARAT_STATUS_OK;
        default:
            g_state.registry.invalid_message_count++;
            return BHARAT_STATUS_ERR_OPCODE;
    }
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    init_telemetrymgr();

    bharat_ipc_endpoint_t ep = service_runtime_create_endpoint(BHARAT_SERVICE_TELEMETRYMGR, 0);
    if (ep == BHARAT_CAP_INVALID_HANDLE) {
        bharat_runtime_log("telemetrymgr: Failed to create endpoint\n");
        return BHARAT_STATUS_ERR_NO_MEMORY;
    }

    bh_service_start_info_t info = {
        .service_id = BHARAT_SERVICE_TELEMETRYMGR,
        .service_name = "telemetrymgr",
        .endpoint = ep
    };

    return bh_service_main(&info);
}
