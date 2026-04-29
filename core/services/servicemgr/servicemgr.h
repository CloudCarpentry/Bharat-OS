#ifndef SERVICEMGR_H
#define SERVICEMGR_H

#include <stdint.h>
#include <stdbool.h>
#include <bharat/uapi/servicemgr/contract.h>
#include <bharat/ipc/ipc.h>

#define MAX_SERVICES 64
#define MAX_RESTART_COUNT 3
#define HEARTBEAT_TIMEOUT_MS 5000
#define DEFAULT_INITIAL_BACKOFF_MS 100
#define DEFAULT_MAX_BACKOFF_MS 5000
#define DEFAULT_BACKOFF_MULTIPLIER 2

typedef struct {
    const char *name;
    uint32_t service_id;
    uint32_t boot_priority;
    uint32_t flags;
    const char **hard_deps;
    uint32_t hard_deps_count;
    const char **soft_deps;
    uint32_t soft_deps_count;
    sm_restart_policy_t restart_policy;
    bool critical;
} bh_service_manifest_entry_t;

typedef struct {
    uint32_t service_id;
    uint64_t incarnation_id;
    sm_service_state_t state;
    uint32_t restart_count;
    uint64_t last_start_ticks;
    uint64_t last_heartbeat_ticks;
    uint64_t next_retry_ticks;
    uint32_t current_backoff_ms;

    const bh_service_manifest_entry_t *manifest;
    bool in_use;
} bh_service_instance_t;

void servicemgr_init(void);
void servicemgr_loop(bharat_ipc_endpoint_t endpoint);

// Internal management
int32_t servicemgr_load_manifest(const bh_service_manifest_entry_t *manifest, uint32_t count);
int32_t servicemgr_validate_dependencies(void);
int32_t servicemgr_start_all(void);

// IPC Handlers
int32_t servicemgr_handle_register(const sm_req_register_t *req, sm_resp_register_t *resp);
int32_t servicemgr_handle_start(const sm_req_start_t *req, sm_resp_start_t *resp);
int32_t servicemgr_handle_stop(const sm_req_stop_t *req, sm_resp_stop_t *resp);
int32_t servicemgr_handle_query(const sm_req_query_t *req, sm_resp_query_t *resp);
int32_t servicemgr_handle_heartbeat(const sm_req_heartbeat_t *req, sm_resp_heartbeat_t *resp);

void servicemgr_check_health(uint64_t current_ticks);
int32_t servicemgr_authorize(uint32_t opcode, const void *req, bharat_cap_handle_t caller_cap);

#endif // SERVICEMGR_H
