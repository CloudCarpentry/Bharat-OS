#include "servicemgr.h"
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <bharat/cap/cap_validate.h>
#include <bharat/uapi/ipc/status.h>

static bh_service_instance_t service_instances[MAX_SERVICES];
static uint32_t instance_count = 0;

static bool is_transition_allowed(sm_service_state_t from, sm_service_state_t to) {
    switch (from) {
        case SM_STATE_CREATED:
            return (to == SM_STATE_STARTING || to == SM_STATE_FAILED);
        case SM_STATE_STARTING:
            return (to == SM_STATE_RUNNING || to == SM_STATE_FAILED || to == SM_STATE_STOPPING);
        case SM_STATE_RUNNING:
            return (to == SM_STATE_DEGRADED || to == SM_STATE_STOPPING || to == SM_STATE_FAILED);
        case SM_STATE_DEGRADED:
            return (to == SM_STATE_RUNNING || to == SM_STATE_STOPPING || to == SM_STATE_FAILED);
        case SM_STATE_STOPPING:
            return (to == SM_STATE_STOPPED || to == SM_STATE_FAILED);
        case SM_STATE_STOPPED:
            return (to == SM_STATE_STARTING || to == SM_STATE_CREATED);
        case SM_STATE_FAILED:
            return (to == SM_STATE_RESTART_BACKOFF || to == SM_STATE_STOPPED);
        case SM_STATE_RESTART_BACKOFF:
            return (to == SM_STATE_STARTING || to == SM_STATE_STOPPED);
        default:
            return false;
    }
}

static int32_t set_service_state(bh_service_instance_t *inst, sm_service_state_t new_state) {
    if (!is_transition_allowed(inst->state, new_state)) {
        printf("servicemgr: Invalid transition for service %u: %d -> %d\n", inst->service_id, inst->state, new_state);
        return BHARAT_IPC_STATUS_ERR_PERM;
    }
    inst->state = new_state;
    return BHARAT_IPC_STATUS_OK;
}

void servicemgr_init(void) {
    memset(service_instances, 0, sizeof(service_instances));
    instance_count = 0;
}

int32_t servicemgr_load_manifest(const bh_service_manifest_entry_t *manifest, uint32_t count) {
    for (uint32_t i = 0; i < count && instance_count < MAX_SERVICES; i++) {
        service_instances[instance_count].manifest = &manifest[i];
        service_instances[instance_count].service_id = manifest[i].service_id;
        service_instances[instance_count].state = SM_STATE_CREATED;
        service_instances[instance_count].incarnation_id = 0;
        service_instances[instance_count].in_use = true;
        service_instances[instance_count].restart_count = 0;
        service_instances[instance_count].current_backoff_ms = DEFAULT_INITIAL_BACKOFF_MS;
        instance_count++;
    }
    return BHARAT_IPC_STATUS_OK;
}

static bh_service_instance_t* find_instance_by_id(uint32_t service_id) {
    for (uint32_t i = 0; i < MAX_SERVICES; i++) {
        if (service_instances[i].in_use && service_instances[i].service_id == service_id) {
            return &service_instances[i];
        }
    }
    return NULL;
}

static bh_service_instance_t* find_instance_by_name(const char *name) {
    for (uint32_t i = 0; i < MAX_SERVICES; i++) {
        if (service_instances[i].in_use && service_instances[i].manifest && service_instances[i].manifest->name && strcmp(service_instances[i].manifest->name, name) == 0) {
            return &service_instances[i];
        }
    }
    return NULL;
}

int32_t servicemgr_validate_dependencies(void) {
    for (uint32_t i = 0; i < MAX_SERVICES; i++) {
        if (!service_instances[i].in_use) continue;

        const bh_service_manifest_entry_t *m = service_instances[i].manifest;
        if (!m) continue;
        for (uint32_t j = 0; j < m->hard_deps_count; j++) {
            if (find_instance_by_name(m->hard_deps[j]) == NULL) {
                return BHARAT_IPC_STATUS_ERR_NOT_FOUND;
            }
        }
    }
    return BHARAT_IPC_STATUS_OK;
}

static int32_t start_service(bh_service_instance_t *inst) {
    if (inst->state == SM_STATE_RUNNING || inst->state == SM_STATE_STARTING) {
        return BHARAT_IPC_STATUS_OK;
    }

    // Check hard dependencies
    const bh_service_manifest_entry_t *m = inst->manifest;
    if (!m) return BHARAT_IPC_STATUS_ERR_INTERNAL;

    for (uint32_t i = 0; i < m->hard_deps_count; i++) {
        bh_service_instance_t *dep = find_instance_by_name(m->hard_deps[i]);
        if (!dep || dep->state != SM_STATE_RUNNING) {
            return BHARAT_IPC_STATUS_ERR_PERM;
        }
    }

    if (set_service_state(inst, SM_STATE_STARTING) != BHARAT_IPC_STATUS_OK) {
        return BHARAT_IPC_STATUS_ERR_PERM;
    }

    inst->incarnation_id++;
    inst->last_start_ticks = 0;
    inst->last_heartbeat_ticks = 0;

    return BHARAT_IPC_STATUS_OK;
}

int32_t servicemgr_start_all(void) {
    bool progress = true;
    while (progress) {
        progress = false;
        for (uint32_t i = 0; i < MAX_SERVICES; i++) {
            if (service_instances[i].in_use && service_instances[i].state == SM_STATE_CREATED) {
                if (start_service(&service_instances[i]) == BHARAT_IPC_STATUS_OK) {
                    progress = true;
                }
            }
        }
    }
    return BHARAT_IPC_STATUS_OK;
}

int32_t servicemgr_handle_heartbeat(const sm_req_heartbeat_t *req, sm_resp_heartbeat_t *resp) {
    bh_service_instance_t *inst = find_instance_by_id(req->service_id);
    if (!inst) {
        resp->status = BHARAT_IPC_STATUS_ERR_NOT_FOUND;
        return BHARAT_IPC_STATUS_ERR_NOT_FOUND;
    }

    if (inst->incarnation_id != req->incarnation_id) {
        printf("servicemgr: Stale incarnation heartbeat rejected for service %u (got %lu, expected %lu)\n",
               inst->service_id, (unsigned long)req->incarnation_id, (unsigned long)inst->incarnation_id);
        resp->status = BHARAT_IPC_STATUS_ERR_PERM;
        return BHARAT_IPC_STATUS_ERR_PERM;
    }

    inst->last_heartbeat_ticks = req->timestamp_ticks;

    resp->status = BHARAT_IPC_STATUS_OK;
    return BHARAT_IPC_STATUS_OK;
}

int32_t servicemgr_handle_signal_ready(const sm_req_heartbeat_t *req, sm_resp_heartbeat_t *resp) {
    bh_service_instance_t *inst = find_instance_by_id(req->service_id);
    if (!inst) {
        resp->status = BHARAT_IPC_STATUS_ERR_NOT_FOUND;
        return BHARAT_IPC_STATUS_ERR_NOT_FOUND;
    }

    if (inst->incarnation_id != req->incarnation_id) {
        resp->status = BHARAT_IPC_STATUS_ERR_PERM;
        return BHARAT_IPC_STATUS_ERR_PERM;
    }

    set_service_state(inst, SM_STATE_RUNNING);
    resp->status = BHARAT_IPC_STATUS_OK;
    return BHARAT_IPC_STATUS_OK;
}

int32_t servicemgr_handle_query(const sm_req_query_t *req, sm_resp_query_t *resp) {
    bh_service_instance_t *inst = find_instance_by_id(req->service_id);
    if (!inst) {
        resp->status = BHARAT_IPC_STATUS_ERR_NOT_FOUND;
        return BHARAT_IPC_STATUS_ERR_NOT_FOUND;
    }

    resp->service_id = inst->service_id;
    resp->incarnation_id = inst->incarnation_id;
    resp->state = inst->state;
    resp->restart_count = inst->restart_count;
    resp->status = BHARAT_IPC_STATUS_OK;
    return BHARAT_IPC_STATUS_OK;
}

void servicemgr_check_health(uint64_t current_ticks) {
    for (uint32_t i = 0; i < MAX_SERVICES; i++) {
        bh_service_instance_t *inst = &service_instances[i];
        if (!inst->in_use) continue;

        if (inst->state == SM_STATE_RUNNING || inst->state == SM_STATE_DEGRADED) {
            if (inst->last_heartbeat_ticks > 0 && current_ticks > inst->last_heartbeat_ticks + HEARTBEAT_TIMEOUT_MS) {
                printf("servicemgr: Service %u timed out\n", inst->service_id);
                set_service_state(inst, SM_STATE_FAILED);

                // Restart logic
                bool allow_restart = false;
                if (!inst->manifest) allow_restart = false;
                else if (inst->manifest->restart_policy == SM_RESTART_POLICY_ALWAYS) allow_restart = true;
                else if (inst->manifest->restart_policy == SM_RESTART_POLICY_ON_FAILURE) allow_restart = true;
                else if (inst->manifest->restart_policy == SM_RESTART_POLICY_BOUNDED_RETRY && inst->restart_count < MAX_RESTART_COUNT) allow_restart = true;

                if (allow_restart) {
                    set_service_state(inst, SM_STATE_RESTART_BACKOFF);
                    inst->restart_count++;
                    inst->next_retry_ticks = current_ticks + inst->current_backoff_ms;
                    // Exponential backoff
                    inst->current_backoff_ms *= DEFAULT_BACKOFF_MULTIPLIER;
                    if (inst->current_backoff_ms > DEFAULT_MAX_BACKOFF_MS) inst->current_backoff_ms = DEFAULT_MAX_BACKOFF_MS;
                }
            }
        } else if (inst->state == SM_STATE_RESTART_BACKOFF) {
            if (current_ticks >= inst->next_retry_ticks) {
                start_service(inst);
            }
        }
    }
}

int32_t servicemgr_authorize(uint32_t opcode, const void *req, bharat_cap_handle_t caller_cap) {
    if (caller_cap == BHARAT_CAP_INVALID_HANDLE) {
        return BHARAT_IPC_STATUS_ERR_PERM;
    }
    // Simple mock authorization
    return BHARAT_IPC_STATUS_OK;
}

void servicemgr_loop(bharat_ipc_endpoint_t endpoint) {
    bharat_ipc_msg_header_t req_header;
    bharat_ipc_msg_header_t resp_header;
    uint8_t payload_buf[1024];
    uint8_t resp_payload_buf[1024];
    uint64_t mock_ticks = 0;

    while (true) {
        int32_t recv_status = bharat_ipc_recv(endpoint, &req_header, payload_buf, sizeof(payload_buf));

        mock_ticks += 100;
        servicemgr_check_health(mock_ticks);

        if (recv_status < 0) continue;

        if (req_header.service_id != SERVICEMGR_SERVICE_ID) continue;

        resp_header = req_header;
        resp_header.flags |= BHARAT_IPC_FLAG_REPLY;

        int32_t dispatch_status = BHARAT_IPC_STATUS_ERR_OPCODE;
        uint32_t resp_size = 0;

        switch (req_header.opcode) {
            case SM_OP_QUERY: {
                if (req_header.payload_size >= sizeof(sm_req_query_t)) {
                    dispatch_status = servicemgr_handle_query((sm_req_query_t*)payload_buf, (sm_resp_query_t*)resp_payload_buf);
                    resp_size = sizeof(sm_resp_query_t);
                }
                break;
            }
            case SM_OP_HEARTBEAT: {
                if (req_header.payload_size >= sizeof(sm_req_heartbeat_t)) {
                    dispatch_status = servicemgr_handle_heartbeat((sm_req_heartbeat_t*)payload_buf, (sm_resp_heartbeat_t*)resp_payload_buf);
                    resp_size = sizeof(sm_resp_heartbeat_t);
                }
                break;
            }
            case SM_OP_SIGNAL_READY: {
                if (req_header.payload_size >= sizeof(sm_req_heartbeat_t)) {
                    dispatch_status = servicemgr_handle_signal_ready((sm_req_heartbeat_t*)payload_buf, (sm_resp_heartbeat_t*)resp_payload_buf);
                    resp_size = sizeof(sm_resp_heartbeat_t);
                }
                break;
            }
        }

        resp_header.payload_size = resp_size;
        resp_header.status = dispatch_status;
        if (req_header.reply_endpoint != BHARAT_CAP_INVALID_HANDLE) {
            bharat_ipc_send(req_header.reply_endpoint, &resp_header, resp_payload_buf);
        }
    }
}
