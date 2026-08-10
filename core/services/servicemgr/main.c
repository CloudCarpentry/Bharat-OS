#include <stdint.h>
#include <stddef.h>
#include <bharat/bh_native.h>
#include <bharat/service/service_runtime.h>
#include <bharat/namesvc/client.h>
#include <bharat/uapi/ipc/status.h>
#include "servicemgr.h"

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

    // Create a dynamic endpoint instead of hardcoded 0x3000 placeholder
    bharat_ipc_endpoint_t my_endpoint = service_runtime_create_endpoint(SERVICEMGR_SERVICE_ID, 0);
    if (my_endpoint == BHARAT_CAP_INVALID_HANDLE) {
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

    servicemgr_loop(my_endpoint);

    return 0;
}
