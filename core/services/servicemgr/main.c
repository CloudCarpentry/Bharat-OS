#include <stdint.h>
#include <stddef.h>
#include <bharat/bh_native.h>
#include <bharat/service/service_runtime.h>
#include <bharat/namesvc/client.h>
#include <bharat/uapi/ipc/status.h>
#include <bharat/cap/cap_validate.h>
#include "servicemgr.h"
#include <ipc_user.h>

static bharat_cap_status_t servicemgr_envelope_resolver(
    bharat_cap_handle_t handle,
    bharat_cap_object_type_t expected_object_type,
    uint64_t expected_object_id,
    uint64_t required_rights,
    const bharat_cap_scope_t *required_scope,
    bharat_cap_validation_result_t *out_result)
{
    (void)expected_object_id;
    (void)required_rights;
    (void)required_scope;
    if (!out_result) return BHARAT_CAP_INVALID;
    out_result->allowed = false;
    out_result->status = BHARAT_CAP_INVALID;
    if (handle == BHARAT_CAP_INVALID_HANDLE || expected_object_type != BHARAT_CAP_OBJ_SERVICE) {
        return BHARAT_CAP_INVALID;
    }
    out_result->allowed = true;
    out_result->status = BHARAT_CAP_OK;
    return BHARAT_CAP_OK;
}

static int32_t prod_now_ns(void *ctx, uint64_t *out_now_ns) {
    (void)ctx;
    bh_time_t t;
    int32_t ret = bh_time_get(BH_CLOCK_MONOTONIC, &t);
    if (ret == 0 && out_now_ns) {
        *out_now_ns = t;
    }
    return ret;
}

static int32_t prod_register_endpoint(void *ctx, const char *name, uint32_t service_id, bharat_ipc_endpoint_t ep) {
    (void)ctx;
    return namesvc_register(name, service_id, ep, 1, 0);
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    (void)bharat_cap_register_authority_resolver(servicemgr_envelope_resolver);

    // Create a dynamic endpoint instead of hardcoded 0x3000 placeholder
    uint32_t send_cap = 0, receive_cap = 0;
    if (ipc_endpoint_create(&send_cap, &receive_cap) != 0 || !send_cap || !receive_cap) {
        return -1;
    }

    bh_servicemgr_clock_ops_t clock_ops = {
        .ctx = NULL,
        .now_ns = prod_now_ns
    };

    bh_service_registry_ops_t registry_ops = {
        .ctx = NULL,
        .register_endpoint = prod_register_endpoint
    };

    bh_servicemgr_dependencies_t deps = {
        .launcher = &g_launcher_processmgr_ops,
        .clock = &clock_ops,
        .registry = &registry_ops
    };

    servicemgr_init(&deps);
    if (prod_register_endpoint(NULL, "bharat.servicemgr", SERVICEMGR_SERVICE_ID, send_cap) != NAMESVC_STATUS_OK)
        return -1;
    /* Publication alone does not establish authenticated handoff readiness. */
    servicemgr_loop(receive_cap);

    return 0;
}
