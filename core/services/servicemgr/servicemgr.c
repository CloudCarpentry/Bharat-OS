#include "servicemgr.h"
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <bharat/uapi/ipc/status.h>
#include <bharat/runtime/runtime.h>
#include <bharat/namesvc/client.h>
#include <bharat/syscalls.h>
#include <bharat/uapi/process_manager/contract.h>

#define STABILITY_WINDOW_NS (2ULL * HEARTBEAT_TIMEOUT_MS * BH_NS_PER_MS)
/* Owned exclusively by the supervisor event loop. No cross-core mutation. */
static bh_servicemgr_t g_servicemgr;

bh_servicemgr_t *servicemgr_get_instance(void) { return &g_servicemgr; }

static int32_t now_ns(uint64_t *now) {
    if (!g_servicemgr.deps.clock || !g_servicemgr.deps.clock->now_ns)
        return BHARAT_IPC_STATUS_ERR_INTERNAL;
    return g_servicemgr.deps.clock->now_ns(g_servicemgr.deps.clock->ctx, now);
}

static void invalidate_stability(void) {
    g_servicemgr.stable_reached = false;
    g_servicemgr.stable_window_active = false;
    g_servicemgr.stable_started_ns = 0;
}

static bh_service_instance_t *find_instance(uint32_t id) {
    for (uint32_t i = 0; i < g_servicemgr.instance_count; ++i)
        if (g_servicemgr.service_instances[i].in_use &&
            g_servicemgr.service_instances[i].service_id == id)
            return &g_servicemgr.service_instances[i];
    return NULL;
}

static void set_state(bh_service_instance_t *inst, sm_service_state_t state) {
    inst->state = state;
    if (inst->graph_rec.critical && state != SM_STATE_RUNNING) invalidate_stability();
}

int32_t servicemgr_init(const bh_servicemgr_dependencies_t *deps) {
    memset(&g_servicemgr, 0, sizeof(g_servicemgr));
    if (deps) g_servicemgr.deps = *deps;
    bharat_runtime_log("BOOT_SERVICE:servicemgr:DECLARED\n");
    return BHARAT_IPC_STATUS_OK;
}

/* Kahn traversal checks both missing edges and cycles without recursive stack use. */
static int32_t validate_graph(bh_service_instance_t *items, uint32_t count) {
    bool done[MAX_SERVICES] = {false};
    uint32_t visited = 0;
    for (uint32_t pass = 0; pass < count; ++pass) {
        bool progress = false;
        for (uint32_t i = 0; i < count; ++i) {
            if (done[i]) continue;
            bool ready = true;
            for (uint32_t d = 0; d < items[i].graph_rec.dependency_count; ++d) {
                uint32_t j = 0;
                for (; j < count; ++j)
                    if (items[j].service_id == items[i].graph_rec.dependencies[d]) break;
                if (j == count) return BHARAT_IPC_STATUS_ERR_NOT_FOUND;
                if (!done[j]) ready = false;
            }
            if (ready) { done[i] = true; ++visited; progress = true; }
        }
        if (!progress) break;
    }
    return visited == count ? BHARAT_IPC_STATUS_OK : BHARAT_IPC_STATUS_ERR_INVALID;
}

int32_t servicemgr_load_manifest(const bh_service_manifest_entry_t *manifest, uint32_t count) {
    if (!manifest || count > MAX_SERVICES || g_servicemgr.instance_count)
        return BHARAT_IPC_STATUS_ERR_INVALID;
    for (uint32_t i = 0; i < count; ++i) {
        if (!manifest[i].name || !manifest[i].service_id ||
            strlen(manifest[i].name) >= sizeof(g_servicemgr.service_instances[i].graph_rec.service_name) ||
            manifest[i].hard_deps_count > SM_HANDOFF_MAX_DEPS ||
            (manifest[i].hard_deps_count && !manifest[i].hard_deps))
            return BHARAT_IPC_STATUS_ERR_INVALID;
        for (uint32_t j = 0; j < i; ++j)
            if (manifest[j].service_id == manifest[i].service_id ||
                strcmp(manifest[j].name, manifest[i].name) == 0)
                return BHARAT_IPC_STATUS_ERR_INVALID;
    }
    for (uint32_t i = 0; i < count; ++i) {
        bh_service_instance_t *inst = &g_servicemgr.service_instances[i];
        inst->manifest = &manifest[i];
        inst->service_id = manifest[i].service_id;
        inst->in_use = true;
        inst->state = SM_STATE_CREATED;
        inst->health_state = SM_HEALTH_STATE_DECLARED;
        inst->current_backoff_ms = DEFAULT_INITIAL_BACKOFF_MS;
        bh_service_graph_record_t *rec = &inst->graph_rec;
        strcpy(rec->service_name, manifest[i].name);
        rec->service_id = inst->service_id;
        rec->executable_id = manifest[i].executable_id;
        rec->priority = manifest[i].priority;
        rec->critical = manifest[i].critical;
        rec->restart_policy = manifest[i].restart_policy;
        rec->retry_limit = MAX_RESTART_COUNT;
        rec->dependency_count = manifest[i].hard_deps_count;
        for (uint32_t d = 0; d < rec->dependency_count; ++d) {
            uint32_t j = 0;
            for (; j < count; ++j)
                if (manifest[i].hard_deps[d] && strcmp(manifest[j].name, manifest[i].hard_deps[d]) == 0) break;
            rec->dependencies[d] = j < count ? manifest[j].service_id : 0;
        }
    }
    g_servicemgr.instance_count = count;
    return BHARAT_IPC_STATUS_OK;
}

int32_t servicemgr_validate_dependencies(void) {
    return validate_graph(g_servicemgr.service_instances, g_servicemgr.instance_count);
}

static int32_t start_service(bh_service_instance_t *inst) {
    if (inst->state == SM_STATE_RUNNING || inst->state == SM_STATE_STARTING) return 0;
    if (inst->state != SM_STATE_CREATED && inst->state != SM_STATE_STOPPED &&
        inst->state != SM_STATE_RESTART_BACKOFF && inst->state != SM_STATE_FAILED)
        return BHARAT_IPC_STATUS_ERR_PERM;
    for (uint32_t d = 0; d < inst->graph_rec.dependency_count; ++d) {
        bh_service_instance_t *dep = find_instance(inst->graph_rec.dependencies[d]);
        if (!dep || dep->state != SM_STATE_RUNNING) return BHARAT_IPC_STATUS_ERR_PERM;
    }
    uint64_t now = 0;
    if (now_ns(&now) != 0) return BHARAT_IPC_STATUS_ERR_INTERNAL;
    if (!g_servicemgr.deps.launcher || !g_servicemgr.deps.launcher->spawn ||
        inst->incarnation_id == UINT64_MAX) {
        set_state(inst, SM_STATE_FAILED);
        return BHARAT_IPC_STATUS_ERR_PERM;
    }
    bh_service_launch_spec_t spec = {.service_id = inst->service_id,
        .executable_id = inst->graph_rec.executable_id, .priority = inst->graph_rec.priority,
        .boot_session_id = g_servicemgr.boot_session_id, .incarnation_id = inst->incarnation_id + 1};
    bh_service_launch_result_t result = {0};
    int32_t rc = g_servicemgr.deps.launcher->spawn(g_servicemgr.deps.launcher->ctx, &spec, &result);
    if (rc != 0 || result.status != 0 || result.process_id == 0) {
        set_state(inst, SM_STATE_FAILED);
        return rc != 0 ? rc : BHARAT_IPC_STATUS_ERR_INTERNAL;
    }
    inst->process_id = result.process_id;
    inst->incarnation_id = spec.incarnation_id;
    inst->heartbeat_sequence = 0;
    inst->stop_requested = false;
    inst->health_state = SM_HEALTH_STATE_SPAWNED;
    inst->last_start_ns = inst->last_heartbeat_ns = now;
    set_state(inst, SM_STATE_STARTING);
    return 0;
}

int32_t servicemgr_start_all(void) {
    int32_t rc = servicemgr_validate_dependencies();
    if (rc != 0) return rc;
    int32_t first_error = 0;
    for (uint32_t i = 0; i < g_servicemgr.instance_count; ++i) {
        bh_service_instance_t *inst = &g_servicemgr.service_instances[i];
        if (inst->state != SM_STATE_CREATED) continue;
        rc = start_service(inst);
        if (rc != 0 && first_error == 0) first_error = rc;
    }
    return first_error;
}

int32_t servicemgr_handle_health_update(const sm_req_health_update_t *req,
                                       sm_resp_health_update_t *resp,
                                       uint64_t authenticated_pid) {
    if (!resp) return BHARAT_IPC_STATUS_ERR_INVALID;
    resp->status = BHARAT_IPC_STATUS_ERR_PERM;
    if (!req || req->struct_size != sizeof(*req) || req->padding ||
        req->health_state > SM_HEALTH_STATE_STOPPED) {
        resp->status = BHARAT_IPC_STATUS_ERR_INVALID;
        return resp->status;
    }
    bh_service_instance_t *inst = find_instance(req->service_id);
    if (!inst) return resp->status = BHARAT_IPC_STATUS_ERR_NOT_FOUND;
    if (!authenticated_pid || authenticated_pid != inst->process_id ||
        req->process_id != inst->process_id || !req->boot_session_id ||
        req->boot_session_id != g_servicemgr.boot_session_id ||
        req->incarnation_id != inst->incarnation_id || !req->incarnation_id ||
        req->heartbeat_sequence <= inst->heartbeat_sequence) return resp->status;
    uint32_t from = inst->health_state, to = req->health_state;
    bool legal = to == from;
    if (from == SM_HEALTH_STATE_DECLARED) legal |= to == SM_HEALTH_STATE_SPAWNED;
    if (from == SM_HEALTH_STATE_SPAWNED) legal |= to == SM_HEALTH_STATE_BOUND;
    if (from == SM_HEALTH_STATE_BOUND) legal |= to == SM_HEALTH_STATE_READY;
    if (from == SM_HEALTH_STATE_READY) legal |= to == SM_HEALTH_STATE_DEGRADED;
    if (from == SM_HEALTH_STATE_DEGRADED) legal |= to == SM_HEALTH_STATE_READY;
    if (to == SM_HEALTH_STATE_FAILED && inst->state != SM_STATE_STOPPED) legal = true;
    if (to == SM_HEALTH_STATE_STOPPED) legal = inst->state == SM_STATE_STOPPING;
    if (!legal || (inst->state != SM_STATE_STARTING && inst->state != SM_STATE_RUNNING &&
        inst->state != SM_STATE_DEGRADED && inst->state != SM_STATE_STOPPING)) return resp->status;
    uint64_t now = 0;
    if (now_ns(&now) != 0 || now < inst->last_heartbeat_ns)
        return resp->status = BHARAT_IPC_STATUS_ERR_INTERNAL;
    inst->heartbeat_sequence = req->heartbeat_sequence;
    inst->last_heartbeat_ns = now;
    inst->health_state = to;
    if (to == SM_HEALTH_STATE_READY) set_state(inst, SM_STATE_RUNNING);
    else if (to == SM_HEALTH_STATE_DEGRADED) set_state(inst, SM_STATE_DEGRADED);
    else if (to == SM_HEALTH_STATE_FAILED) set_state(inst, SM_STATE_FAILED);
    else if (to == SM_HEALTH_STATE_STOPPED) set_state(inst, SM_STATE_STOPPED);
    return resp->status = BHARAT_IPC_STATUS_OK;
}

int32_t servicemgr_handle_query(const sm_req_query_t *req, sm_resp_query_t *resp) {
    if (!req || !resp) return BHARAT_IPC_STATUS_ERR_INVALID;
    bh_service_instance_t *inst = find_instance(req->service_id);
    if (!inst) return resp->status = BHARAT_IPC_STATUS_ERR_NOT_FOUND;
    *resp = (sm_resp_query_t){.service_id = inst->service_id, .incarnation_id = inst->incarnation_id,
        .state = inst->state, .restart_count = inst->restart_count, .status = 0};
    return 0;
}

int32_t servicemgr_handle_register(const sm_req_register_t *req, sm_resp_register_t *resp) {
    if (!req || !resp) return BHARAT_IPC_STATUS_ERR_INVALID;
    resp->status = BHARAT_IPC_STATUS_ERR_INVALID;
    size_t len = 0;
    while (len < sizeof(req->service_name) && req->service_name[len]) ++len;
    if (!len || len >= sizeof(g_servicemgr.service_instances[0].graph_rec.service_name) ||
        req->restart_policy > SM_RESTART_POLICY_SAFE_MODE_ESCALATION ||
        g_servicemgr.instance_count == MAX_SERVICES) return resp->status;
    uint32_t id = 1;
    for (uint32_t i = 0; i < g_servicemgr.instance_count; ++i) {
        bh_service_instance_t *other = &g_servicemgr.service_instances[i];
        if (strcmp(other->graph_rec.service_name, req->service_name) == 0)
            return resp->status = BHARAT_IPC_STATUS_ERR_PERM;
        if (other->service_id >= id) {
            if (other->service_id == UINT32_MAX) return resp->status;
            id = other->service_id + 1;
        }
    }
    bh_service_instance_t *inst = &g_servicemgr.service_instances[g_servicemgr.instance_count++];
    memset(inst, 0, sizeof(*inst));
    inst->in_use = true;
    inst->service_id = inst->graph_rec.service_id = id;
    strcpy(inst->graph_rec.service_name, req->service_name);
    inst->graph_rec.restart_policy = req->restart_policy;
    inst->graph_rec.critical = req->critical;
    inst->graph_rec.retry_limit = MAX_RESTART_COUNT;
    inst->current_backoff_ms = DEFAULT_INITIAL_BACKOFF_MS;
    resp->service_id = id;
    return resp->status = 0;
}

int32_t servicemgr_handle_start(const sm_req_start_t *req, sm_resp_start_t *resp) {
    if (!req || !resp) return BHARAT_IPC_STATUS_ERR_INVALID;
    bh_service_instance_t *inst = find_instance(req->service_id);
    return resp->status = inst ? start_service(inst) : BHARAT_IPC_STATUS_ERR_NOT_FOUND;
}

int32_t servicemgr_handle_stop(const sm_req_stop_t *req, sm_resp_stop_t *resp) {
    if (!req || !resp) return BHARAT_IPC_STATUS_ERR_INVALID;
    bh_service_instance_t *inst = find_instance(req->service_id);
    if (!inst) return resp->status = BHARAT_IPC_STATUS_ERR_NOT_FOUND;
    if ((inst->state != SM_STATE_STARTING && inst->state != SM_STATE_RUNNING &&
         inst->state != SM_STATE_DEGRADED) || !inst->process_id ||
        !g_servicemgr.deps.launcher || !g_servicemgr.deps.launcher->request_stop)
        return resp->status = BHARAT_IPC_STATUS_ERR_PERM;
    int32_t rc = g_servicemgr.deps.launcher->request_stop(g_servicemgr.deps.launcher->ctx, inst->process_id);
    if (rc != 0) return resp->status = rc;
    /* Request acceptance is not proof of process termination. */
    set_state(inst, SM_STATE_STOPPING);
    inst->stop_requested = true;
    return resp->status = 0;
}

void servicemgr_check_health(uint64_t current_ns) {
    bool all_ready = true, has_critical = false;
    for (uint32_t i = 0; i < g_servicemgr.instance_count; ++i) {
        bh_service_instance_t *inst = &g_servicemgr.service_instances[i];
        if (inst->stop_requested && inst->process_id && g_servicemgr.deps.launcher &&
            g_servicemgr.deps.launcher->query) {
            uint32_t process_state = PM_STATE_UNKNOWN;
            int32_t query_rc = g_servicemgr.deps.launcher->query(g_servicemgr.deps.launcher->ctx,
                inst->process_id, &process_state);
            if (query_rc == 0 && process_state == PM_STATE_EXITED) {
                inst->process_id = 0;
                inst->stop_requested = false;
                if (inst->state == SM_STATE_STOPPING) set_state(inst, SM_STATE_STOPPED);
            }
        }
        uint64_t timeout = (uint64_t)HEARTBEAT_TIMEOUT_MS * BH_NS_PER_MS;
        if (inst->state == SM_STATE_STARTING && inst->graph_rec.ready_deadline_ms)
            timeout = (uint64_t)inst->graph_rec.ready_deadline_ms * BH_NS_PER_MS;
        uint64_t since = inst->state == SM_STATE_STARTING ? inst->last_start_ns : inst->last_heartbeat_ns;
        if ((inst->state == SM_STATE_STARTING || inst->state == SM_STATE_RUNNING || inst->state == SM_STATE_DEGRADED) &&
            current_ns >= since && current_ns - since > timeout)
            set_state(inst, SM_STATE_FAILED);
        if (inst->state == SM_STATE_FAILED) {
            uint32_t policy = inst->graph_rec.restart_policy;
            bool retry = policy == SM_RESTART_POLICY_ALWAYS || policy == SM_RESTART_POLICY_ON_FAILURE ||
                (policy == SM_RESTART_POLICY_BOUNDED_RETRY && inst->restart_count < inst->graph_rec.retry_limit);
            if (retry && inst->restart_count < UINT32_MAX) {
                /* Never spawn a replacement until stopping the old process succeeded. */
                int32_t rc = 0;
                if (inst->process_id) {
                    if (!g_servicemgr.deps.launcher || !g_servicemgr.deps.launcher->request_stop) rc = BHARAT_IPC_STATUS_ERR_PERM;
                    else if (!inst->stop_requested) {
                        rc = g_servicemgr.deps.launcher->request_stop(g_servicemgr.deps.launcher->ctx, inst->process_id);
                        if (rc == 0) inst->stop_requested = true;
                    }
                    /* Await a later PM query proving EXITED before replacement. */
                    rc = BHARAT_IPC_STATUS_ERR_PERM;
                }
                uint64_t delay = (uint64_t)inst->current_backoff_ms * BH_NS_PER_MS;
                if (rc == 0 && delay <= UINT64_MAX - current_ns) {
                    ++inst->restart_count;
                    inst->next_retry_ns = current_ns + delay;
                    inst->current_backoff_ms = inst->current_backoff_ms > DEFAULT_MAX_BACKOFF_MS / DEFAULT_BACKOFF_MULTIPLIER
                        ? DEFAULT_MAX_BACKOFF_MS : inst->current_backoff_ms * DEFAULT_BACKOFF_MULTIPLIER;
                    set_state(inst, SM_STATE_RESTART_BACKOFF);
                }
            }
        } else if (inst->state == SM_STATE_RESTART_BACKOFF && current_ns >= inst->next_retry_ns) {
            start_service(inst);
        }
        if (inst->graph_rec.critical) {
            has_critical = true;
            if (inst->state != SM_STATE_RUNNING) all_ready = false;
        }
    }
    if (!g_servicemgr.handoff_committed || !has_critical || !all_ready) {
        invalidate_stability();
    } else if (!g_servicemgr.stable_window_active) {
        g_servicemgr.stable_window_active = true;
        g_servicemgr.stable_started_ns = current_ns;
    } else if (current_ns < g_servicemgr.stable_started_ns) {
        invalidate_stability();
    } else if (!g_servicemgr.stable_reached && current_ns - g_servicemgr.stable_started_ns >= STABILITY_WINDOW_NS) {
        g_servicemgr.stable_reached = true;
        bharat_runtime_log("BOOT_RUNTIME: STABLE\n");
    }
}

static int32_t discard_stage(int32_t status) {
    g_servicemgr.handoff_begun = false;
    g_servicemgr.handoff_record_count = 0;
    memset(g_servicemgr.staged_instances, 0, sizeof(g_servicemgr.staged_instances));
    return status;
}

int32_t servicemgr_stage_begin(const sm_handoff_begin_t *req) {
    if (!req || req->abi_major != 1 || req->abi_minor != 0 || req->struct_size != sizeof(*req))
        return discard_stage(BHARAT_IPC_STATUS_ERR_VERSION);
    if (!req->boot_session_id || req->flags || !req->selected_service_count ||
        req->selected_service_count > MAX_SERVICES || req->already_running_count > req->selected_service_count ||
        req->boot_session_id == g_servicemgr.boot_session_id)
        return discard_stage(BHARAT_IPC_STATUS_ERR_INVALID);
    discard_stage(0);
    g_servicemgr.staged_boot_session_id = req->boot_session_id;
    g_servicemgr.staged_profile = req->selected_profile;
    g_servicemgr.expected_record_count = req->selected_service_count;
    g_servicemgr.handoff_begun = true;
    return 0;
}

int32_t servicemgr_stage_service(const sm_handoff_service_t *req) {
    if (!req || !g_servicemgr.handoff_begun || req->record_index != g_servicemgr.handoff_record_count ||
        g_servicemgr.handoff_record_count >= g_servicemgr.expected_record_count || !req->service_id ||
        !req->service_name[0] || !memchr(req->service_name, 0, sizeof(req->service_name)) ||
        req->dependency_count > SM_HANDOFF_MAX_DEPS || req->critical > 1 ||
        req->restart_policy > SM_RESTART_POLICY_SAFE_MODE_ESCALATION ||
        req->reserved[0] || req->reserved[1] || req->reserved[2] ||
        (req->process_id && !req->incarnation_id)) return discard_stage(BHARAT_IPC_STATUS_ERR_INVALID);
    for (uint32_t i = 0; i < g_servicemgr.handoff_record_count; ++i) {
        bh_service_instance_t *other = &g_servicemgr.staged_instances[i];
        if (other->service_id == req->service_id || strcmp(other->graph_rec.service_name, req->service_name) == 0 ||
            (req->process_id && other->process_id == req->process_id))
            return discard_stage(BHARAT_IPC_STATUS_ERR_INVALID);
    }
    for (uint32_t d = 0; d < req->dependency_count; ++d) {
        if (!req->dependencies[d] || req->dependencies[d] == req->service_id)
            return discard_stage(BHARAT_IPC_STATUS_ERR_INVALID);
        for (uint32_t j = 0; j < d; ++j)
            if (req->dependencies[d] == req->dependencies[j]) return discard_stage(BHARAT_IPC_STATUS_ERR_INVALID);
    }
    bh_service_instance_t *inst = &g_servicemgr.staged_instances[g_servicemgr.handoff_record_count++];
    inst->in_use = true;
    inst->service_id = req->service_id;
    inst->process_id = req->process_id;
    inst->incarnation_id = req->incarnation_id;
    /* Adoption never manufactures READY from a claimed observed_state. */
    inst->state = req->process_id ? SM_STATE_STARTING : SM_STATE_CREATED;
    inst->health_state = req->process_id ? SM_HEALTH_STATE_SPAWNED : SM_HEALTH_STATE_DECLARED;
    inst->current_backoff_ms = DEFAULT_INITIAL_BACKOFF_MS;
    bh_service_graph_record_t *rec = &inst->graph_rec;
    strcpy(rec->service_name, req->service_name);
    rec->service_id = req->service_id; rec->executable_id = req->executable_id;
    rec->priority = req->priority; rec->boot_class = req->boot_class;
    rec->restart_policy = req->restart_policy; rec->start_deadline_ms = req->start_deadline_ms;
    rec->ready_deadline_ms = req->ready_deadline_ms; rec->retry_limit = req->retry_limit;
    rec->critical = req->critical; rec->dependency_count = req->dependency_count;
    for (uint32_t d = 0; d < rec->dependency_count; ++d) rec->dependencies[d] = req->dependencies[d];
    return 0;
}

int32_t servicemgr_stage_commit(const sm_handoff_commit_t *req) {
    if (!req || !g_servicemgr.handoff_begun || req->boot_session_id != g_servicemgr.staged_boot_session_id ||
        req->record_count != g_servicemgr.expected_record_count || req->record_count != g_servicemgr.handoff_record_count)
        return discard_stage(BHARAT_IPC_STATUS_ERR_INVALID);
    int32_t rc = validate_graph(g_servicemgr.staged_instances, req->record_count);
    uint64_t now = 0;
    if (rc != 0) return discard_stage(rc);
    if (now_ns(&now) != 0) return discard_stage(BHARAT_IPC_STATUS_ERR_INTERNAL);
    for (uint32_t i = 0; i < req->record_count; ++i) {
        g_servicemgr.staged_instances[i].last_start_ns = now;
        g_servicemgr.staged_instances[i].last_heartbeat_ns = now;
    }
    /* Atomic relative to this single-threaded event loop. Trusted process identity
     * verification must precede this internal call; wire RPC is disabled below. */
    memcpy(g_servicemgr.service_instances, g_servicemgr.staged_instances, sizeof(g_servicemgr.service_instances));
    g_servicemgr.instance_count = req->record_count;
    g_servicemgr.boot_session_id = g_servicemgr.staged_boot_session_id;
    g_servicemgr.selected_profile = g_servicemgr.staged_profile;
    g_servicemgr.handoff_committed = true;
    invalidate_stability();
    discard_stage(0);
    return 0;
}

int32_t servicemgr_authorize(uint32_t opcode, const void *req, bharat_cap_handle_t caller_cap) {
    (void)opcode; (void)req; (void)caller_cap;
    /* IPC currently transfers only a reply SEND cap, with no authenticated PID
     * or separate authority cap. A wire integer cannot authorize any operation.
     * No test resolver is installed as a production fallback. */
    return BHARAT_IPC_STATUS_ERR_PERM;
}

int32_t servicemgr_poll_once(bh_servicemgr_t *mgr, uint64_t wait_deadline_ns) {
    if (mgr != &g_servicemgr) return BHARAT_IPC_STATUS_ERR_INVALID;
    uint64_t now = 0;
    if (now_ns(&now) != 0) return BHARAT_IPC_STATUS_ERR_INTERNAL;
    servicemgr_check_health(now);
    uint64_t timeout = 100 * BH_NS_PER_MS;
    if (wait_deadline_ns > now && wait_deadline_ns - now < timeout) timeout = wait_deadline_ns - now;
    bharat_ipc_msg_header_t header = {0};
    union { sm_handoff_begin_t begin; sm_req_health_update_t health; uint8_t bytes[128]; } payload = {0};
    int32_t rc = bharat_ipc_recv_ex(mgr->my_endpoint, &header, &payload, sizeof(payload), timeout);
    if (rc != 0) return rc;
    /* Validate exact sizes before reading any field, including for authorization. */
    size_t expected = 0;
    switch (header.opcode) {
    case SM_OP_REGISTER: expected = sizeof(sm_req_register_t); break;
    case SM_OP_START: expected = sizeof(sm_req_start_t); break;
    case SM_OP_STOP: expected = sizeof(sm_req_stop_t); break;
    case SM_OP_QUERY: expected = sizeof(sm_req_query_t); break;
    case SM_OP_HEALTH_UPDATE: expected = sizeof(sm_req_health_update_t); break;
    case SM_OP_HANDOFF_BEGIN: expected = sizeof(sm_handoff_begin_t); break;
    case SM_OP_HANDOFF_SERVICE: expected = sizeof(sm_handoff_service_t); break;
    case SM_OP_HANDOFF_COMMIT: expected = sizeof(sm_handoff_commit_t); break;
    default: return BHARAT_IPC_STATUS_ERR_OPCODE;
    }
    if (header.payload_size != expected) return BHARAT_IPC_STATUS_ERR_LENGTH;
    if (header.opcode == SM_OP_HANDOFF_BEGIN &&
        (payload.begin.abi_major != 1 || payload.begin.abi_minor || payload.begin.struct_size != sizeof(payload.begin)))
        return BHARAT_IPC_STATUS_ERR_VERSION;
    if (header.opcode == SM_OP_HEALTH_UPDATE && payload.health.struct_size != sizeof(payload.health))
        return BHARAT_IPC_STATUS_ERR_LENGTH;
    /* Do not reply to an unverified numeric reply_endpoint. Actual transfer
     * metadata is not exposed by recv_ex independently of the wire header. */
    return servicemgr_authorize(header.opcode, &payload, header.capability_transfer);
}

void servicemgr_loop(bharat_ipc_endpoint_t endpoint) {
    g_servicemgr.my_endpoint = endpoint;
    /* Publication needs a distinct SEND capability. The caller supplied RECEIVE
     * only; fail closed until main provides and registers a real SEND endpoint. */
    while (true) {
        uint64_t now = 0;
        if (now_ns(&now) != 0) return;
        servicemgr_poll_once(&g_servicemgr, now);
        bharat_sched_yield();
    }
}
