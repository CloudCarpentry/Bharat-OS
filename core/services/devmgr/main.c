#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <bharat/service/service_runtime.h>
#include "devmgr_core.h"

bharat_status_t bh_service_handle_msg(bh_service_ctx_t *ctx, const bh_msg_t *msg) {
    return devmgr_handle_message(ctx, msg);
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    devmgr_registry_init();

    bharat_ipc_endpoint_t ep = service_runtime_create_endpoint(BHARAT_SERVICE_DEVMGR, 0);
    if (ep == BHARAT_CAP_INVALID_HANDLE) {
        return -1;
    }

    bh_service_start_info_t info = {
        .service_id = BHARAT_SERVICE_DEVMGR,
        .service_name = "devmgr",
        .endpoint = ep,
        .user_data = NULL
    };

    return bh_service_main(&info);
}