#ifndef SERVICEMGR_H
#define SERVICEMGR_H

#include <stdint.h>
#include <stdbool.h>
#include <bharat/uapi/servicemgr/contract.h>
#include <bharat/uapi/servicemgr/handoff.h>
#include <bharat/ipc/ipc.h>
#include <uapi/time/time.h>

#define MAX_SERVICES 64
#define MAX_RESTART_COUNT 3
#define HEARTBEAT_TIMEOUT_MS 5000
#define DEFAULT_INITIAL_BACKOFF_MS 100
#define DEFAULT_MAX_BACKOFF_MS 5000
#define DEFAULT_BACKOFF_MULTIPLIER 2

typedef struct {
    const char *name;
    uint32_t service_id;
    uint32_t executable_id;
    uint32_t priority;
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
    char service_name[16];
    uint32_t executable_id;
    uint32_t priority;
    uint32_t boot_class;
    uint32_t restart_policy;
    uint32_t start_deadline_ms;
    uint32_t ready_deadline_ms;
    uint32_t retry_limit;
    uint32_t critical;
    uint32_t observed_state;
    uint32_t process_id;
    uint64_t incarnation_id;
    uint32_t dependency_count;
    uint32_t dependencies[SM_HANDOFF_MAX_DEPS];
} bh_service_graph_record_t;

typedef struct {
    uint32_t service_id;
    uint64_t incarnation_id;
    sm_service_state_t state;
    uint32_t restart_count;
    uint64_t last_start_ns;
    uint64_t last_heartbeat_ns;
    uint64_t next_retry_ns;
    uint32_t current_backoff_ms;

    uint32_t process_id;
    bh_service_graph_record_t graph_rec;
    const bh_service_manifest_entry_t *manifest;
    bool in_use;
} bh_service_instance_t;

// Dependency injection structs
typedef struct {
    uint32_t service_id;
    uint32_t executable_id;
    uint32_t priority;
    uint64_t boot_session_id;
    uint64_t incarnation_id;
} bh_service_launch_spec_t;

typedef struct {
    uint32_t process_id;
    int32_t status;
} bh_service_launch_result_t;

typedef struct {
    void *ctx;

    int32_t (*spawn)(
        void *ctx,
        const bh_service_launch_spec_t *spec,
        bh_service_launch_result_t *out);

    int32_t (*request_stop)(
        void *ctx,
        uint32_t process_id);

    int32_t (*query)(
        void *ctx,
        uint32_t process_id,
        uint32_t *out_state);
} bh_service_launcher_ops_t;

typedef struct {
    void *ctx;
    int32_t (*now_ns)(void *ctx, uint64_t *out_now_ns);
} bh_servicemgr_clock_ops_t;

typedef struct {
    void *ctx;
    int32_t (*register_endpoint)(void *ctx, const char *name, uint32_t service_id, bharat_ipc_endpoint_t ep);
} bh_service_registry_ops_t;

typedef struct {
    const bh_service_launcher_ops_t *launcher;
    const bh_servicemgr_clock_ops_t *clock;
    const bh_service_registry_ops_t *registry;
} bh_servicemgr_dependencies_t;

typedef struct {
    bh_service_instance_t service_instances[MAX_SERVICES];
    uint32_t instance_count;
    bh_servicemgr_dependencies_t deps;
    uint64_t boot_session_id;
    uint32_t selected_profile;
    bool handoff_begun;
    bool handoff_committed;
    uint32_t handoff_record_count;
    uint32_t expected_record_count;
    bharat_ipc_endpoint_t my_endpoint;
    bool stable_reached;
} bh_servicemgr_t;

int32_t servicemgr_init(const bh_servicemgr_dependencies_t *deps);
int32_t servicemgr_poll_once(bh_servicemgr_t *mgr, uint64_t wait_deadline_ns);
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
int32_t servicemgr_handle_signal_ready(const sm_req_heartbeat_t *req, sm_resp_heartbeat_t *resp);

void servicemgr_check_health(uint64_t current_ticks);
int32_t servicemgr_authorize(uint32_t opcode, const void *req, bharat_cap_handle_t caller_cap);

// Global servicemgr instance accessor (for production)
bh_servicemgr_t *servicemgr_get_instance(void);

// Production launcher declaration
extern const bh_service_launcher_ops_t g_launcher_processmgr_ops;

#endif // SERVICEMGR_H
