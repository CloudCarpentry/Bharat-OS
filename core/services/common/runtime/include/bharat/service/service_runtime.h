#ifndef BHARAT_SERVICE_RUNTIME_H
#define BHARAT_SERVICE_RUNTIME_H

#include <stdint.h>
#include <stdbool.h>
#include <bharat/uapi/service_status.h>
#include <bharat/uapi/ipc/contract.h>
#include <bharat/uapi/servicemgr/contract.h>
#include <bharat/uapi/services/service_ids.h>
#include <bharat/ipc/ipc.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    bharat_service_id_t service_id;
    uint64_t incarnation_id;
    bharat_ipc_endpoint_t endpoint;
    void *user_data;
    bool should_exit;
} bh_service_ctx_t;

typedef struct {
    bharat_service_id_t service_id;
    const char *service_name;
    bharat_ipc_endpoint_t endpoint;
    void *user_data;
} bh_service_start_info_t;

typedef struct {
    bharat_ipc_msg_header_t header;
    const void *payload;
    uint32_t payload_size;
} bh_msg_t;

/**
 * @brief Common service main entry point.
 */
int bh_service_main(const bh_service_start_info_t *info);

/**
 * @brief Poll for a single event/message.
 */
bharat_status_t bh_service_poll_once(bh_service_ctx_t *ctx);

/**
 * @brief Handle an incoming IPC message.
 */
bharat_status_t bh_service_handle_msg(bh_service_ctx_t *ctx, const bh_msg_t *msg);

/**
 * @brief Request the service to shut down gracefully.
 */
bharat_status_t bh_service_request_shutdown(bh_service_ctx_t *ctx);

/**
 * @brief Signal readiness to the service manager.
 */
bharat_status_t bh_service_signal_ready(bh_service_ctx_t *ctx);

/**
 * @brief Send a heartbeat to the service manager.
 */
bharat_status_t bh_service_send_heartbeat(bh_service_ctx_t *ctx, uint32_t health_state);

/**
 * @brief Create an IPC endpoint for a service.
 *
 * @param service_id The service ID requesting the endpoint.
 * @param flags Creation flags.
 * @return bharat_ipc_endpoint_t The received endpoint handle or BHARAT_CAP_INVALID_HANDLE.
 */
bharat_ipc_endpoint_t service_runtime_create_endpoint(bharat_service_id_t service_id, uint32_t flags);

/**
 * @brief Binds a service to the namesvc bootstrap endpoint.
 *
 * This is a transitional Phase A helper for namesvc itself.
 *
 * @param endpoint The endpoint handle to bind as the bootstrap namesvc.
 * @return bharat_status_t BHARAT_STATUS_OK on success.
 */
bharat_status_t service_runtime_bind_namesvc_bootstrap(bharat_ipc_endpoint_t endpoint);

#ifdef __cplusplus
}
#endif

#endif // BHARAT_SERVICE_RUNTIME_H
