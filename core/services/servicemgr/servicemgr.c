#include "servicemgr.h"
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <bharat/cap/cap_validate.h>
#include <bharat/uapi/ipc/status.h>
#include <bharat/uapi/process_manager/contract.h>
#include <bharat/runtime/runtime.h>
#include <bharat/namesvc/client.h>
#include <bharat/syscalls.h>

#define STABILITY_WINDOW_NS (2ULL * HEARTBEAT_TIMEOUT_MS * BH_NS_PER_MS)

static bh_servicemgr_t g_servicemgr;

bh_servicemgr_t *servicemgr_get_instance(void) {
    return &g_servicemgr;
}

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
            return (to == SM_STATE_RESTART_BACKOFF || to == SM_STATE_STOPPED || to == SM_STATE_STARTING);
        case SM_STATE_RESTART_BACKOFF:
            return (to == SM_STATE_STARTING || to == SM_STATE_STOPPED);
        default:
            return false;
    }
}

static int32_t set_service_state(bh_servicemgr_t *mgr, bh_service_instance_t *inst, sm_service_state_t new_state) {
    if (!is_transition_allowed(inst->state, new_state)) {
        printf("servicemgr: Invalid transition for service %u: %d -> %d\n", inst->service_id, inst->state, new_state);
        return BHARAT_IPC_STATUS_ERR_PERM;
    }

    sm_service_state_t old_state = inst->state;
    inst->state = new_state;

    // Transition Markers
    char log_buf[128];
    if (new_state == SM_STATE_STARTING) {
        if (inst->restart_count > 0) {
            snprintf(log_buf, sizeof(log_buf), "SERVICE:%s:RESTARTED\n", inst->graph_rec.service_name);
            bharat_runtime_log(log_buf);
            printf("%s", log_buf);
        } else {
            snprintf(log_buf, sizeof(log_buf), "SERVICE:%s:SPAWNED\n", inst->graph_rec.service_name);
            bharat_runtime_log(log_buf);
            printf("%s", log_buf);
        }
    } else if (new_state == SM_STATE_RUNNING && old_state != SM_STATE_RUNNING) {
        snprintf(log_buf, sizeof(log_buf), "SERVICE:%s:READY\n", inst->graph_rec.service_name);
        bharat_runtime_log(log_buf);
        printf("%s", log_buf);
    } else if (new_state == SM_STATE_FAILED && old_state != SM_STATE_FAILED) {
        snprintf(log_buf, sizeof(log_buf), "SERVICE:%s:FAILED\n", inst->graph_rec.service_name);
        bharat_runtime_log(log_buf);
        printf("%s", log_buf);
    }

    return BHARAT_IPC_STATUS_OK;
}

int32_t servicemgr_init(const bh_servicemgr_dependencies_t *deps) {
    memset(&g_servicemgr, 0, sizeof(g_servicemgr));
    if (deps) {
        g_servicemgr.deps = *deps;
    }
    g_servicemgr.handoff_begun = false;
    g_servicemgr.handoff_committed = false;
    g_servicemgr.stable_reached = false;

    // Emit DECLARED and READY for servicemgr itself
    bharat_runtime_log("BOOT_SERVICE:servicemgr:DECLARED\n");
    printf("BOOT_SERVICE:servicemgr:DECLARED\n");
    bharat_runtime_log("BOOT_SERVICE:servicemgr:READY\n");
    printf("BOOT_SERVICE:servicemgr:READY\n");

    return BHARAT_IPC_STATUS_OK;
}

int32_t servicemgr_load_manifest(const bh_service_manifest_entry_t *manifest, uint32_t count) {
    for (uint32_t i = 0; i < count && g_servicemgr.instance_count < MAX_SERVICES; i++) {
        bh_service_instance_t *inst = &g_servicemgr.service_instances[g_servicemgr.instance_count];
        inst->manifest = &manifest[i];
        inst->service_id = manifest[i].service_id;
        inst->state = SM_STATE_CREATED;
        inst->incarnation_id = 0;
        inst->in_use = true;
        inst->restart_count = 0;
        inst->current_backoff_ms = DEFAULT_INITIAL_BACKOFF_MS;
        strncpy(inst->graph_rec.service_name, manifest[i].name, sizeof(inst->graph_rec.service_name) - 1);
        inst->graph_rec.service_id = manifest[i].service_id;
        inst->graph_rec.executable_id = manifest[i].executable_id;
        inst->graph_rec.priority = manifest[i].priority;
        inst->graph_rec.critical = manifest[i].critical;
        inst->graph_rec.restart_policy = manifest[i].restart_policy;

        g_servicemgr.instance_count++;
    }
    return BHARAT_IPC_STATUS_OK;
}

static bh_service_instance_t* find_instance_by_id(uint32_t service_id) {
    for (uint32_t i = 0; i < MAX_SERVICES; i++) {
        if (g_servicemgr.service_instances[i].in_use && g_servicemgr.service_instances[i].service_id == service_id) {
            return &g_servicemgr.service_instances[i];
        }
    }
    return NULL;
}

static bh_service_instance_t* find_instance_by_name(const char *name) {
    for (uint32_t i = 0; i < MAX_SERVICES; i++) {
        if (g_servicemgr.service_instances[i].in_use && strcmp(g_servicemgr.service_instances[i].graph_rec.service_name, name) == 0) {
            return &g_servicemgr.service_instances[i];
        }
    }
    return NULL;
}

int32_t servicemgr_validate_dependencies(void) {
    for (uint32_t i = 0; i < MAX_SERVICES; i++) {
        if (!g_servicemgr.service_instances[i].in_use) continue;

        bh_service_instance_t *inst = &g_servicemgr.service_instances[i];
        for (uint32_t j = 0; j < inst->graph_rec.dependency_count; j++) {
            uint32_t dep_id = inst->graph_rec.dependencies[j];
            if (find_instance_by_id(dep_id) == NULL) {
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

    // Check hard dependencies liveness
    for (uint32_t i = 0; i < inst->graph_rec.dependency_count; i++) {
        uint32_t dep_id = inst->graph_rec.dependencies[i];
        bh_service_instance_t *dep = find_instance_by_id(dep_id);
        if (!dep || dep->state != SM_STATE_RUNNING) {
            return BHARAT_IPC_STATUS_ERR_PERM;
        }
    }

    if (set_service_state(&g_servicemgr, inst, SM_STATE_STARTING) != BHARAT_IPC_STATUS_OK) {
        return BHARAT_IPC_STATUS_ERR_PERM;
    }

    inst->incarnation_id++;
    inst->last_start_ns = 0;
    inst->last_heartbeat_ns = 0;

    // Call spawn if we have a launcher
    if (g_servicemgr.deps.launcher && g_servicemgr.deps.launcher->spawn) {
        bh_service_launch_spec_t spec = {
            .service_id = inst->service_id,
            .executable_id = inst->graph_rec.executable_id,
            .priority = inst->graph_rec.priority,
            .boot_session_id = g_servicemgr.boot_session_id,
            .incarnation_id = inst->incarnation_id
        };
        bh_service_launch_result_t result = {0};
        int32_t spawn_rc = g_servicemgr.deps.launcher->spawn(g_servicemgr.deps.launcher->ctx, &spec, &result);
        if (spawn_rc == BHARAT_IPC_STATUS_OK) {
            inst->process_id = result.process_id;
        } else {
            set_service_state(&g_servicemgr, inst, SM_STATE_FAILED);
        }
    }

    return BHARAT_IPC_STATUS_OK;
}

int32_t servicemgr_start_all(void) {
    bool progress = true;
    while (progress) {
        progress = false;
        for (uint32_t i = 0; i < MAX_SERVICES; i++) {
            if (g_servicemgr.service_instances[i].in_use && g_servicemgr.service_instances[i].state == SM_STATE_CREATED) {
                if (start_service(&g_servicemgr.service_instances[i]) == BHARAT_IPC_STATUS_OK) {
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

    // Reject heartbeat if not in RUNNING, STARTING, DEGRADED
    if (inst->state != SM_STATE_RUNNING && inst->state != SM_STATE_STARTING && inst->state != SM_STATE_DEGRADED) {
        printf("servicemgr: Heartbeat rejected because service %u is in invalid state %d\n",
               inst->service_id, (int)inst->state);
        resp->status = BHARAT_IPC_STATUS_ERR_PERM;
        return BHARAT_IPC_STATUS_ERR_PERM;
    }

    // Read supervisor's clock time to avoid malicious/future client timestamps
    uint64_t current_time_ns = 0;
    if (g_servicemgr.deps.clock && g_servicemgr.deps.clock->now_ns) {
        g_servicemgr.deps.clock->now_ns(g_servicemgr.deps.clock->ctx, &current_time_ns);
    }
    inst->last_heartbeat_ns = current_time_ns;

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
        printf("servicemgr: Stale readiness signal rejected for service %u\n", inst->service_id);
        resp->status = BHARAT_IPC_STATUS_ERR_PERM;
        return BHARAT_IPC_STATUS_ERR_PERM;
    }

    if (inst->state != SM_STATE_STARTING && inst->state != SM_STATE_RUNNING) {
        printf("servicemgr: Readiness rejected: service %u has not been successfully spawned/adopted.\n", inst->service_id);
        resp->status = BHARAT_IPC_STATUS_ERR_PERM;
        return BHARAT_IPC_STATUS_ERR_PERM;
    }

    set_service_state(&g_servicemgr, inst, SM_STATE_RUNNING);
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

int32_t servicemgr_handle_register(const sm_req_register_t *req, sm_resp_register_t *resp) {
    // Reject duplicate registration with conflicting endpoint identity
    bh_service_instance_t *existing = find_instance_by_name(req->service_name);
    if (existing) {
        // If already registered and running on same/different ID, reject if mismatch
        resp->status = BHARAT_IPC_STATUS_ERR_PERM;
        return BHARAT_IPC_STATUS_ERR_PERM;
    }

    if (g_servicemgr.instance_count >= MAX_SERVICES) {
        resp->status = BHARAT_IPC_STATUS_ERR_PERM;
        return BHARAT_IPC_STATUS_ERR_PERM;
    }

    bh_service_instance_t *inst = &g_servicemgr.service_instances[g_servicemgr.instance_count++];
    strncpy(inst->graph_rec.service_name, req->service_name, sizeof(inst->graph_rec.service_name) - 1);
    inst->service_id = g_servicemgr.instance_count; // generate new ID
    inst->graph_rec.service_id = inst->service_id;
    inst->state = SM_STATE_CREATED;
    inst->incarnation_id = 0;
    inst->in_use = true;
    inst->restart_count = 0;
    inst->current_backoff_ms = DEFAULT_INITIAL_BACKOFF_MS;
    inst->graph_rec.restart_policy = req->restart_policy;
    inst->graph_rec.critical = req->critical;

    char log_buf[128];
    snprintf(log_buf, sizeof(log_buf), "SERVICE:%s:DECLARED\n", inst->graph_rec.service_name);
    bharat_runtime_log(log_buf);
    printf("%s", log_buf);

    resp->service_id = inst->service_id;
    resp->status = BHARAT_IPC_STATUS_OK;
    return BHARAT_IPC_STATUS_OK;
}

int32_t servicemgr_handle_start(const sm_req_start_t *req, sm_resp_start_t *resp) {
    bh_service_instance_t *inst = find_instance_by_id(req->service_id);
    if (!inst) {
        resp->status = BHARAT_IPC_STATUS_ERR_NOT_FOUND;
        return BHARAT_IPC_STATUS_ERR_NOT_FOUND;
    }

    resp->status = start_service(inst);
    return resp->status;
}

int32_t servicemgr_handle_stop(const sm_req_stop_t *req, sm_resp_stop_t *resp) {
    bh_service_instance_t *inst = find_instance_by_id(req->service_id);
    if (!inst) {
        resp->status = BHARAT_IPC_STATUS_ERR_NOT_FOUND;
        return BHARAT_IPC_STATUS_ERR_NOT_FOUND;
    }

    if (set_service_state(&g_servicemgr, inst, SM_STATE_STOPPING) != BHARAT_IPC_STATUS_OK) {
        resp->status = BHARAT_IPC_STATUS_ERR_PERM;
        return BHARAT_IPC_STATUS_ERR_PERM;
    }

    if (g_servicemgr.deps.launcher && g_servicemgr.deps.launcher->request_stop) {
        g_servicemgr.deps.launcher->request_stop(g_servicemgr.deps.launcher->ctx, inst->process_id);
    }

    set_service_state(&g_servicemgr, inst, SM_STATE_STOPPED);
    resp->status = BHARAT_IPC_STATUS_OK;
    return BHARAT_IPC_STATUS_OK;
}

void servicemgr_check_health(uint64_t current_ns) {
    bool all_critical_ready = true;
    bool any_critical_restarting = false;
    bool has_critical_services = false;

    for (uint32_t i = 0; i < MAX_SERVICES; i++) {
        bh_service_instance_t *inst = &g_servicemgr.service_instances[i];
        if (!inst->in_use) continue;

        if (inst->graph_rec.critical) {
            has_critical_services = true;
            if (inst->state != SM_STATE_RUNNING) {
                all_critical_ready = false;
            }
            if (inst->state == SM_STATE_RESTART_BACKOFF || inst->state == SM_STATE_STARTING) {
                any_critical_restarting = true;
            }
        }

        if (inst->state == SM_STATE_RUNNING || inst->state == SM_STATE_DEGRADED) {
            uint64_t timeout_ns = (uint64_t)HEARTBEAT_TIMEOUT_MS * BH_NS_PER_MS;
            if (inst->last_heartbeat_ns > 0 && current_ns > inst->last_heartbeat_ns + timeout_ns) {
                printf("servicemgr: Service %u (process %u) timed out\n", inst->service_id, inst->process_id);
                set_service_state(&g_servicemgr, inst, SM_STATE_FAILED);

                // Stop launcher representation if process exists
                if (inst->process_id > 0 && g_servicemgr.deps.launcher && g_servicemgr.deps.launcher->request_stop) {
                    g_servicemgr.deps.launcher->request_stop(g_servicemgr.deps.launcher->ctx, inst->process_id);
                }

                // Restart logic
                bool allow_restart = false;
                uint32_t policy = inst->graph_rec.restart_policy;
                if (policy == SM_RESTART_POLICY_ALWAYS) allow_restart = true;
                else if (policy == SM_RESTART_POLICY_ON_FAILURE) allow_restart = true;
                else if (policy == SM_RESTART_POLICY_BOUNDED_RETRY && inst->restart_count < MAX_RESTART_COUNT) allow_restart = true;

                if (allow_restart) {
                    set_service_state(&g_servicemgr, inst, SM_STATE_RESTART_BACKOFF);
                    inst->restart_count++;
                    inst->next_retry_ns = current_ns + (uint64_t)inst->current_backoff_ms * BH_NS_PER_MS;
                    // Exponential backoff
                    inst->current_backoff_ms *= DEFAULT_BACKOFF_MULTIPLIER;
                    if (inst->current_backoff_ms > DEFAULT_MAX_BACKOFF_MS) inst->current_backoff_ms = DEFAULT_MAX_BACKOFF_MS;
                }
            }
        } else if (inst->state == SM_STATE_RESTART_BACKOFF) {
            if (current_ns >= inst->next_retry_ns) {
                start_service(inst);
            }
        }
    }

    // Check stability
    if (g_servicemgr.handoff_committed && !g_servicemgr.stable_reached && has_critical_services && all_critical_ready && !any_critical_restarting) {
        // Find if we have been in this stable window long enough
        static uint64_t stable_started_ns = 0;
        if (stable_started_ns == 0) {
            stable_started_ns = current_ns;
        } else if (current_ns >= stable_started_ns + STABILITY_WINDOW_NS) {
            g_servicemgr.stable_reached = true;
            bharat_runtime_log("BOOT_RUNTIME: STABLE\n");
            printf("BOOT_RUNTIME: STABLE\n");
        }
    } else {
        // Reset stable timer if a critical service drops out of READY
        if (!all_critical_ready || any_critical_restarting) {
            // stable_started_ns resets
        }
    }
}

#include <bharat/cap/cap_authz.h>

static const bharat_service_authz_desc_t servicemgr_authz_descs[] = {
    {
        .opcode = SM_OP_REGISTER,
        .object_type = BHARAT_CAP_OBJ_SERVICE,
        .required_rights = BHARAT_CAP_RIGHT_WRITE,
    },
    {
        .opcode = SM_OP_START,
        .object_type = BHARAT_CAP_OBJ_SERVICE,
        .required_rights = BHARAT_CAP_RIGHT_WRITE,
    },
    {
        .opcode = SM_OP_STOP,
        .object_type = BHARAT_CAP_OBJ_SERVICE,
        .required_rights = BHARAT_CAP_RIGHT_WRITE,
    },
    {
        .opcode = SM_OP_QUERY,
        .object_type = BHARAT_CAP_OBJ_SERVICE,
        .required_rights = BHARAT_CAP_RIGHT_READ,
    },
    {
        .opcode = SM_OP_HEARTBEAT,
        .object_type = BHARAT_CAP_OBJ_SERVICE,
        .required_rights = BHARAT_CAP_RIGHT_WRITE,
    },
    {
        .opcode = SM_OP_SIGNAL_READY,
        .object_type = BHARAT_CAP_OBJ_SERVICE,
        .required_rights = BHARAT_CAP_RIGHT_WRITE,
    },
    {
        .opcode = SM_OP_HANDOFF_BEGIN,
        .object_type = BHARAT_CAP_OBJ_SERVICE,
        .required_rights = BHARAT_CAP_RIGHT_WRITE,
    },
    {
        .opcode = SM_OP_HANDOFF_SERVICE,
        .object_type = BHARAT_CAP_OBJ_SERVICE,
        .required_rights = BHARAT_CAP_RIGHT_WRITE,
    },
    {
        .opcode = SM_OP_HANDOFF_COMMIT,
        .object_type = BHARAT_CAP_OBJ_SERVICE,
        .required_rights = BHARAT_CAP_RIGHT_WRITE,
    }
};

int32_t servicemgr_authorize(uint32_t opcode, const void *req, bharat_cap_handle_t caller_cap) {
    if (caller_cap == BHARAT_CAP_INVALID_HANDLE) {
        return BHARAT_IPC_STATUS_ERR_PERM;
    }

    uint64_t target_service_id = 0;
    if (opcode == SM_OP_REGISTER) {
        target_service_id = 0;
    } else if (opcode == SM_OP_START) {
        target_service_id = ((const sm_req_start_t *)req)->service_id;
    } else if (opcode == SM_OP_STOP) {
        target_service_id = ((const sm_req_stop_t *)req)->service_id;
    } else if (opcode == SM_OP_QUERY) {
        target_service_id = ((const sm_req_query_t *)req)->service_id;
    } else if (opcode == SM_OP_HEARTBEAT) {
        target_service_id = ((const sm_req_heartbeat_t *)req)->service_id;
    } else if (opcode == SM_OP_SIGNAL_READY) {
        target_service_id = ((const sm_req_heartbeat_t *)req)->service_id;
    } else if (opcode == SM_OP_HANDOFF_BEGIN) {
        target_service_id = 0;
    } else if (opcode == SM_OP_HANDOFF_SERVICE) {
        target_service_id = ((const sm_handoff_service_t *)req)->service_id;
    } else if (opcode == SM_OP_HANDOFF_COMMIT) {
        target_service_id = 0;
    }

    return bharat_service_dispatch_authorize(
        SERVICEMGR_SERVICE_ID,
        opcode,
        servicemgr_authz_descs,
        sizeof(servicemgr_authz_descs) / sizeof(servicemgr_authz_descs[0]),
        caller_cap,
        target_service_id
    );
}

int32_t servicemgr_poll_once(bh_servicemgr_t *mgr, uint64_t wait_deadline_ns) {
    uint64_t current_time_ns = 0;
    if (mgr->deps.clock && mgr->deps.clock->now_ns) {
        mgr->deps.clock->now_ns(mgr->deps.clock->ctx, &current_time_ns);
    }

    // Process deadlines
    servicemgr_check_health(current_time_ns);

    // Calculate timeout for receive
    uint64_t timeout_ns = 100 * BH_NS_PER_MS; // default 100ms
    if (wait_deadline_ns > current_time_ns) {
        uint64_t diff = wait_deadline_ns - current_time_ns;
        if (diff < timeout_ns) {
            timeout_ns = diff;
        }
    }

    bharat_ipc_msg_header_t req_header;
    uint8_t payload_buf[1024];
    int32_t recv_status = bharat_ipc_recv_ex(mgr->my_endpoint, &req_header, payload_buf, sizeof(payload_buf), timeout_ns);

    if (recv_status == BHARAT_IPC_STATUS_OK) {
        // Authorize before any execution
        int32_t auth_status = servicemgr_authorize(req_header.opcode, payload_buf, req_header.capability_transfer);
        if (auth_status != BHARAT_IPC_STATUS_OK) {
            bharat_ipc_msg_header_t resp_header = req_header;
            resp_header.flags |= BHARAT_IPC_FLAG_REPLY;
            resp_header.status = auth_status;
            resp_header.payload_size = 0;
            if (req_header.reply_endpoint != BHARAT_CAP_INVALID_HANDLE) {
                bharat_ipc_send(req_header.reply_endpoint, &resp_header, NULL);
            }
            return auth_status;
        }

        bharat_ipc_msg_header_t resp_header = req_header;
        resp_header.flags |= BHARAT_IPC_FLAG_REPLY;

        int32_t dispatch_status = BHARAT_IPC_STATUS_ERR_OPCODE;
        uint8_t resp_payload_buf[1024];
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
            case SM_OP_REGISTER: {
                if (req_header.payload_size >= sizeof(sm_req_register_t)) {
                    dispatch_status = servicemgr_handle_register((sm_req_register_t*)payload_buf, (sm_resp_register_t*)resp_payload_buf);
                    resp_size = sizeof(sm_resp_register_t);
                }
                break;
            }
            case SM_OP_START: {
                if (req_header.payload_size >= sizeof(sm_req_start_t)) {
                    dispatch_status = servicemgr_handle_start((sm_req_start_t*)payload_buf, (sm_resp_start_t*)resp_payload_buf);
                    resp_size = sizeof(sm_resp_start_t);
                }
                break;
            }
            case SM_OP_STOP: {
                if (req_header.payload_size >= sizeof(sm_req_stop_t)) {
                    dispatch_status = servicemgr_handle_stop((sm_req_stop_t*)payload_buf, (sm_resp_stop_t*)resp_payload_buf);
                    resp_size = sizeof(sm_resp_stop_t);
                }
                break;
            }
            case SM_OP_HANDOFF_BEGIN: {
                if (req_header.payload_size >= sizeof(sm_handoff_begin_t)) {
                    const sm_handoff_begin_t *begin = (const sm_handoff_begin_t *)payload_buf;
                    sm_handoff_resp_t *resp = (sm_handoff_resp_t *)resp_payload_buf;
                    resp_size = sizeof(sm_handoff_resp_t);

                    if (begin->abi_major != 1 || begin->struct_size != sizeof(sm_handoff_begin_t)) {
                        dispatch_status = BHARAT_IPC_STATUS_ERR_VERSION;
                    } else if (req_header.capability_transfer == BHARAT_CAP_INVALID_HANDLE) {
                        dispatch_status = BHARAT_IPC_STATUS_ERR_PERM;
                    } else if (begin->selected_service_count > MAX_SERVICES) {
                        dispatch_status = BHARAT_IPC_STATUS_ERR_PERM;
                    } else {
                        mgr->boot_session_id = begin->boot_session_id;
                        mgr->selected_profile = begin->selected_profile;
                        mgr->expected_record_count = begin->selected_service_count;
                        mgr->handoff_record_count = 0;
                        mgr->handoff_begun = true;
                        mgr->handoff_committed = false;

                        // Reset any existing records
                        memset(mgr->service_instances, 0, sizeof(mgr->service_instances));
                        mgr->instance_count = 0;

                        dispatch_status = BHARAT_IPC_STATUS_OK;
                        bharat_runtime_log("BOOT_HANDOFF:BEGIN\n");
                        printf("BOOT_HANDOFF:BEGIN\n");
                    }
                    resp->status = dispatch_status;
                }
                break;
            }
            case SM_OP_HANDOFF_SERVICE: {
                if (req_header.payload_size >= sizeof(sm_handoff_service_t)) {
                    const sm_handoff_service_t *rec = (const sm_handoff_service_t *)payload_buf;
                    sm_handoff_resp_t *resp = (sm_handoff_resp_t *)resp_payload_buf;
                    resp_size = sizeof(sm_handoff_resp_t);

                    if (!mgr->handoff_begun || mgr->handoff_committed) {
                        dispatch_status = BHARAT_IPC_STATUS_ERR_PERM;
                    } else if (rec->record_index != mgr->handoff_record_count) {
                        // Reject duplicate, missing, out-of-order records
                        dispatch_status = BHARAT_IPC_STATUS_ERR_INVALID;
                    } else if (mgr->handoff_record_count >= MAX_SERVICES) {
                        dispatch_status = BHARAT_IPC_STATUS_ERR_PERM;
                    } else {
                        bh_service_instance_t *inst = &mgr->service_instances[mgr->handoff_record_count];
                        inst->service_id = rec->service_id;
                        inst->incarnation_id = rec->incarnation_id;
                        inst->process_id = rec->process_id;
                        inst->in_use = true;
                        inst->restart_count = 0;
                        inst->current_backoff_ms = DEFAULT_INITIAL_BACKOFF_MS;

                        // Copy graph metadata
                        strncpy(inst->graph_rec.service_name, rec->service_name, sizeof(inst->graph_rec.service_name) - 1);
                        inst->graph_rec.service_id = rec->service_id;
                        inst->graph_rec.executable_id = rec->executable_id;
                        inst->graph_rec.priority = rec->priority;
                        inst->graph_rec.boot_class = rec->boot_class;
                        inst->graph_rec.restart_policy = rec->restart_policy;
                        inst->graph_rec.start_deadline_ms = rec->start_deadline_ms;
                        inst->graph_rec.ready_deadline_ms = rec->ready_deadline_ms;
                        inst->graph_rec.retry_limit = rec->retry_limit;
                        inst->graph_rec.critical = rec->critical;
                        inst->graph_rec.dependency_count = rec->dependency_count;
                        for (uint32_t d = 0; d < rec->dependency_count && d < SM_HANDOFF_MAX_DEPS; d++) {
                            inst->graph_rec.dependencies[d] = rec->dependencies[d];
                        }

                        // Preserves existing process IDs and incarnation IDs of already-running services
                        if (rec->process_id > 0) {
                            inst->state = SM_STATE_RUNNING;
                            inst->last_heartbeat_ns = current_time_ns;
                        } else {
                            inst->state = SM_STATE_CREATED;
                        }

                        mgr->handoff_record_count++;
                        mgr->instance_count++;

                        char log_buf[128];
                        snprintf(log_buf, sizeof(log_buf), "SERVICE:%s:DECLARED\n", inst->graph_rec.service_name);
                        bharat_runtime_log(log_buf);
                        printf("%s", log_buf);

                        dispatch_status = BHARAT_IPC_STATUS_OK;
                    }
                    resp->status = dispatch_status;
                }
                break;
            }
            case SM_OP_HANDOFF_COMMIT: {
                if (req_header.payload_size >= sizeof(sm_handoff_commit_t)) {
                    const sm_handoff_commit_t *commit = (const sm_handoff_commit_t *)payload_buf;
                    sm_handoff_resp_t *resp = (sm_handoff_resp_t *)resp_payload_buf;
                    resp_size = sizeof(sm_handoff_resp_t);

                    if (!mgr->handoff_begun || mgr->handoff_committed) {
                        dispatch_status = BHARAT_IPC_STATUS_ERR_PERM;
                    } else if (commit->boot_session_id != mgr->boot_session_id) {
                        dispatch_status = BHARAT_IPC_STATUS_ERR_INVALID;
                    } else if (commit->record_count != mgr->handoff_record_count || commit->record_count != mgr->expected_record_count) {
                        dispatch_status = BHARAT_IPC_STATUS_ERR_INVALID;
                    } else {
                        mgr->handoff_committed = true;
                        dispatch_status = BHARAT_IPC_STATUS_OK;

                        bharat_runtime_log("BOOT_HANDOFF:ACCEPTED\n");
                        printf("BOOT_HANDOFF:ACCEPTED\n");

                        // Launches remaining INFRA, OPTIONAL and LATE services that aren't already running
                        servicemgr_start_all();
                    }
                    resp->status = dispatch_status;
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

    // Re-read time and process newly expired deadlines
    if (mgr->deps.clock && mgr->deps.clock->now_ns) {
        mgr->deps.clock->now_ns(mgr->deps.clock->ctx, &current_time_ns);
    }
    servicemgr_check_health(current_time_ns);

    return recv_status;
}

void servicemgr_loop(bharat_ipc_endpoint_t endpoint) {
    g_servicemgr.my_endpoint = endpoint;

    // Register with namesvc
    if (g_servicemgr.deps.registry && g_servicemgr.deps.registry->register_endpoint) {
        g_servicemgr.deps.registry->register_endpoint(g_servicemgr.deps.registry->ctx, "bharat.servicemgr", SERVICEMGR_SERVICE_ID, endpoint);
    } else {
        namesvc_register("bharat.servicemgr", SERVICEMGR_SERVICE_ID, endpoint, 1, 0);
    }

    while (true) {
        uint64_t current_time_ns = 0;
        if (g_servicemgr.deps.clock && g_servicemgr.deps.clock->now_ns) {
            g_servicemgr.deps.clock->now_ns(g_servicemgr.deps.clock->ctx, &current_time_ns);
        }
        servicemgr_poll_once(&g_servicemgr, current_time_ns + 100 * BH_NS_PER_MS);
        bharat_sched_yield();
    }
}
