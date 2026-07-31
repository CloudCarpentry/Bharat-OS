#include "../../../core/services/servicemgr/servicemgr.h"
#include <bharat/uapi/ipc/status.h>
#include <bharat/uapi/process_manager/contract.h>
#include <stddef.h>
#include <string.h>

#define MAX_FAKE_PROCESSES 128

typedef struct {
    uint32_t process_id;
    uint32_t service_id;
    uint32_t executable_id;
    uint32_t priority;
    uint64_t boot_session_id;
    uint64_t incarnation_id;
    uint32_t state; // PM_STATE_*
    bool active;
} fake_proc_t;

static fake_proc_t fake_processes[MAX_FAKE_PROCESSES];
static uint32_t fake_next_pid = 100;
static uint64_t fake_current_time_ns = 0;

void fake_launcher_init(void) {
    memset(fake_processes, 0, sizeof(fake_processes));
    fake_next_pid = 100;
    fake_current_time_ns = 0;
}

void fake_launcher_set_time(uint64_t now_ns) {
    fake_current_time_ns = now_ns;
}

void fake_launcher_advance_time(uint64_t delta_ns) {
    fake_current_time_ns += delta_ns;
}

void fake_launcher_set_process_state(uint32_t process_id, uint32_t state) {
    for (int i = 0; i < MAX_FAKE_PROCESSES; i++) {
        if (fake_processes[i].active && fake_processes[i].process_id == process_id) {
            fake_processes[i].state = state;
            break;
        }
    }
}

static int32_t fake_spawn(
    void *ctx,
    const bh_service_launch_spec_t *spec,
    bh_service_launch_result_t *out)
{
    (void)ctx;
    if (!spec || !out) return BHARAT_IPC_STATUS_ERR_INVALID;

    for (int i = 0; i < MAX_FAKE_PROCESSES; i++) {
        if (!fake_processes[i].active) {
            fake_processes[i].active = true;
            fake_processes[i].process_id = fake_next_pid++;
            fake_processes[i].service_id = spec->service_id;
            fake_processes[i].executable_id = spec->executable_id;
            fake_processes[i].priority = spec->priority;
            fake_processes[i].boot_session_id = spec->boot_session_id;
            fake_processes[i].incarnation_id = spec->incarnation_id;
            fake_processes[i].state = PM_STATE_RUNNING;

            out->process_id = fake_processes[i].process_id;
            out->status = BHARAT_IPC_STATUS_OK;
            return BHARAT_IPC_STATUS_OK;
        }
    }
    return BHARAT_IPC_STATUS_ERR_INTERNAL;
}

static int32_t fake_request_stop(
    void *ctx,
    uint32_t process_id)
{
    (void)ctx;
    for (int i = 0; i < MAX_FAKE_PROCESSES; i++) {
        if (fake_processes[i].active && fake_processes[i].process_id == process_id) {
            fake_processes[i].state = PM_STATE_STOPPING;
            return BHARAT_IPC_STATUS_OK;
        }
    }
    return BHARAT_IPC_STATUS_ERR_NOT_FOUND;
}

static int32_t fake_query(
    void *ctx,
    uint32_t process_id,
    uint32_t *out_state)
{
    (void)ctx;
    if (!out_state) return BHARAT_IPC_STATUS_ERR_INVALID;

    for (int i = 0; i < MAX_FAKE_PROCESSES; i++) {
        if (fake_processes[i].active && fake_processes[i].process_id == process_id) {
            *out_state = fake_processes[i].state;
            return BHARAT_IPC_STATUS_OK;
        }
    }
    return BHARAT_IPC_STATUS_ERR_NOT_FOUND;
}

const bh_service_launcher_ops_t fake_launcher_ops = {
    .ctx = NULL,
    .spawn = fake_spawn,
    .request_stop = fake_request_stop,
    .query = fake_query
};

static int32_t fake_now_ns(void *ctx, uint64_t *out_now_ns) {
    (void)ctx;
    if (!out_now_ns) return BHARAT_IPC_STATUS_ERR_INVALID;
    *out_now_ns = fake_current_time_ns;
    return BHARAT_IPC_STATUS_OK;
}

const bh_servicemgr_clock_ops_t fake_clock_ops = {
    .ctx = NULL,
    .now_ns = fake_now_ns
};

static int32_t fake_register_endpoint(void *ctx, const char *name, uint32_t service_id, bharat_ipc_endpoint_t ep) {
    (void)ctx;
    (void)name;
    (void)service_id;
    (void)ep;
    return BHARAT_IPC_STATUS_OK;
}

const bh_service_registry_ops_t fake_registry_ops = {
    .ctx = NULL,
    .register_endpoint = fake_register_endpoint
};
