#include "init_runtime.h"
#include "init_handoff.h"
#include "init_contract.h"
#include "init_graph.h"
#include <bharat/runtime/runtime.h>
#include <errno.h>
#include <stdio.h>

static bool is_required_boot_class(init_boot_class_t cls) {
    return (cls == BOOT_CLASS_CORE || cls == BOOT_CLASS_INFRA);
}

static bool filter_service(const init_service_desc_t *desc, const init_boot_context_t *ctx) {
    if (!(desc->profile_mask & (uint64_t)ctx->profile)) {
        return false;
    }

    if (desc->board_mask != (uint32_t)BHARAT_INIT_BOARD_ANY && !(desc->board_mask & ctx->board_id)) {
        return false;
    }

    if (desc->personality_mask != BHARAT_INIT_PERSONALITY_ANY && !(desc->personality_mask & (1 << ctx->personality_id))) {
        return false;
    }

    if ((desc->required_caps & ctx->capability_mask) != desc->required_caps) {
        return false;
    }

    return true;
}

static init_service_runtime_t* find_runtime_by_id(init_runtime_t *rt, init_service_id_t id) {
    if (id <= INIT_SVC_NONE || id >= INIT_SERVICE_ID_MAX) return NULL;
    return &rt->services[id];
}

static bool deps_ready(init_runtime_t *rt, const init_service_desc_t *desc) {
    if (desc->dep_count == 0 || desc->deps == NULL) {
        return true;
    }

    for (uint8_t i = 0; i < desc->dep_count; i++) {
        init_service_id_t dep_id = desc->deps[i];
        if (dep_id == INIT_SVC_NONE) continue;

        init_service_runtime_t *dep_rt = find_runtime_by_id(rt, dep_id);
        if (!dep_rt) {
            return false;
        }

        if (dep_rt->state != INIT_SERVICE_STATE_READY) {
            return false;
        }
    }

    return true;
}

static void try_launch_services(init_runtime_t *rt, init_boot_class_t target_class) {
    for (size_t i = 0; i < rt->manifest_count; i++) {
        init_service_id_t id = rt->service_order[i];
        init_service_runtime_t *sr = &rt->services[id];

        if (sr->desc == NULL) continue;
        if (sr->desc->boot_class != target_class) continue;

        if (sr->state != INIT_SERVICE_STATE_PENDING && sr->state != INIT_SERVICE_STATE_WAITING_DEPS) {
            continue;
        }

        if (!deps_ready(rt, sr->desc)) {
            sr->state = INIT_SERVICE_STATE_WAITING_DEPS;
            continue;
        }

        sr->state = INIT_SERVICE_STATE_LAUNCH_REQUESTED;

        int err = -1;
        if (sr->desc->probe_fn) {
            if (!sr->desc->probe_fn(&rt->boot_ctx)) {
                sr->state = INIT_SERVICE_STATE_SKIPPED;
                continue;
            }
        }

        if (sr->desc->bootstrap_hint_fn) {
            init_launch_result_t res = {0};
            err = sr->desc->bootstrap_hint_fn(&rt->boot_ctx, &res);
        } else if (sr->desc->start_fn) {
            err = sr->desc->start_fn(NULL);
        } else {
            err = 0;
        }

        sr->last_error = err;

        if (err == 0) {
            sr->state = INIT_SERVICE_STATE_READY;
            sr->observed_ready = true;
        } else {
            sr->state = INIT_SERVICE_STATE_FAILED;
        }
    }
}

static bool any_failed_in_class(const init_runtime_t *rt, init_boot_class_t cls) {
    for (size_t i = 0; i < rt->manifest_count; i++) {
        init_service_id_t id = rt->service_order[i];
        const init_service_runtime_t *sr = &rt->services[id];
        if (sr->desc == NULL || sr->desc->boot_class != cls) continue;

        if (sr->state == INIT_SERVICE_STATE_FAILED && sr->required_for_boot) {
            return true;
        }
    }
    return false;
}

static bool all_ready_in_class(const init_runtime_t *rt, init_boot_class_t cls) {
    for (size_t i = 0; i < rt->manifest_count; i++) {
        init_service_id_t id = rt->service_order[i];
        const init_service_runtime_t *sr = &rt->services[id];
        if (sr->desc == NULL || sr->desc->boot_class != cls) continue;

        if (sr->state != INIT_SERVICE_STATE_READY && sr->state != INIT_SERVICE_STATE_SKIPPED) {
            if (sr->required_for_boot) return false;
        }
    }
    return true;
}

int init_runtime_run(init_boot_context_t *ctx) {
    init_runtime_t rt;
    __builtin_memset(&rt, 0, sizeof(rt));
    rt.boot_ctx = *ctx;

    init_event_queue_init(&rt.event_queue);

    rt.phase = INIT_PHASE_CONTEXT_READY;

    // Validate Kernel Health
    rt.phase = INIT_PHASE_KERNEL_HEALTH_VALIDATED;
    if (ctx->kernel_health.level == INIT_KERNEL_HEALTH_UNSAFE) {
        rt.outcome = INIT_BOOT_OUTCOME_SAFE_MODE;
        rt.failure_class = INIT_FAIL_PROFILE;
        ctx->safe_mode_requested = true;
        goto finish;
    }

    rt.phase = INIT_PHASE_PROFILE_SELECTED;
    rt.phase = INIT_PHASE_PERSONALITY_SELECTED;

    // 1. Filter manifest
    rt.phase = INIT_PHASE_MANIFEST_SELECTED;
    init_service_desc_t filtered_manifest[INIT_SERVICE_ID_MAX];
    size_t filtered_count = 0;

    for (size_t i = 0; i < g_init_manifest_count; i++) {
        if (filtered_count >= INIT_SERVICE_ID_MAX) break;

        if (filter_service(&g_init_manifest[i], ctx)) {
            filtered_manifest[filtered_count] = g_init_manifest[i];

            init_service_id_t id = g_init_manifest[i].id;
            rt.service_order[rt.manifest_count] = id;
            rt.services[id].desc = &g_init_manifest[i];
            rt.services[id].state = INIT_SERVICE_STATE_PENDING;
            rt.services[id].attempts = 0;
            rt.services[id].last_error = 0;

            if (g_init_manifest[i].policy == INIT_SERVICE_REQUIRED || is_required_boot_class(g_init_manifest[i].boot_class)) {
                rt.services[id].required_for_boot = true;
            }
            rt.manifest_count++;
            filtered_count++;
        }
    }

    init_graph_result_t graph_rc = init_graph_validate(filtered_manifest, filtered_count, ctx);
    if (graph_rc != INIT_GRAPH_OK) {
        rt.failure_class = init_graph_failure_to_init_failure(graph_rc);
        rt.outcome = INIT_BOOT_OUTCOME_SAFE_MODE;
        ctx->safe_mode_requested = true;
        goto finish;
    }

    rt.phase = INIT_PHASE_GRAPH_VALIDATED;

    // 2. CORE Class
    rt.phase = INIT_PHASE_CORE_STARTING;
    try_launch_services(&rt, BOOT_CLASS_CORE);
    if (any_failed_in_class(&rt, BOOT_CLASS_CORE)) {
        rt.failure_class = INIT_FAIL_LAUNCH;
        rt.outcome = INIT_BOOT_OUTCOME_SAFE_MODE;
        ctx->safe_mode_requested = true;
        goto finish;
    }
    if (!all_ready_in_class(&rt, BOOT_CLASS_CORE)) {
        rt.failure_class = INIT_FAIL_TIMEOUT;
        rt.outcome = INIT_BOOT_OUTCOME_SAFE_MODE;
        goto finish;
    }
    rt.phase = INIT_PHASE_CORE_READY;

    // 3. INFRA Class
    rt.phase = INIT_PHASE_INFRA_STARTING;
    try_launch_services(&rt, BOOT_CLASS_INFRA);
    if (any_failed_in_class(&rt, BOOT_CLASS_INFRA)) {
        rt.outcome = INIT_BOOT_OUTCOME_DEGRADED;
    }
    rt.phase = INIT_PHASE_INFRA_READY;

    // 4. OPTIONAL / LATE / DIAGNOSTIC (best effort)
    rt.phase = INIT_PHASE_OPTIONAL_STARTING;
    try_launch_services(&rt, BOOT_CLASS_OPTIONAL);
    try_launch_services(&rt, BOOT_CLASS_LATE);
    if (ctx->diagnostics_requested || ctx->safe_mode_requested) {
        try_launch_services(&rt, BOOT_CLASS_DIAGNOSTIC);
    }

    rt.outcome = (rt.outcome == INIT_BOOT_OUTCOME_DEGRADED) ? INIT_BOOT_OUTCOME_DEGRADED : INIT_BOOT_OUTCOME_SUCCESS;

finish:
    // Status Report
    init_status_report(rt.services, INIT_SERVICE_ID_MAX);

    // Handoff
    rt.phase = INIT_PHASE_HANDOFF_PREPARED;
    int handoff_res = init_handoff_to_supervisor(ctx);
    if (handoff_res == 0) {
        rt.phase = INIT_PHASE_HANDOFF_COMPLETE;
    } else {
        rt.outcome = INIT_BOOT_OUTCOME_HANDOFF_FAILED;
    }

    return (rt.outcome == INIT_BOOT_OUTCOME_SUCCESS || rt.outcome == INIT_BOOT_OUTCOME_DEGRADED) ? 0 : -EFAULT;
}
