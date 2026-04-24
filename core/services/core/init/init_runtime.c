#include "init_runtime.h"
#include "init_handoff.h"
#include "init_contract.h"
#include <bharat/runtime/runtime.h>
#include <errno.h>

static bool is_required_boot_class(init_boot_class_t cls) {
    return (cls == BOOT_CLASS_CORE || cls == BOOT_CLASS_INFRA);
}

static bool filter_service(const init_service_desc_t *desc, const init_boot_context_t *ctx) {
    if (!(desc->profile_mask & ctx->profile)) {
        return false;
    }

    if (desc->board_mask != BHARAT_INIT_BOARD_ANY && !(desc->board_mask & ctx->board_id)) {
        return false;
    }

    if (desc->personality_mask != BHARAT_INIT_PERSONALITY_ANY && !(desc->personality_mask & ctx->personality_id)) {
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

static void try_issue_bootstrap_hints(init_runtime_t *rt) {
    for (size_t i = 0; i < rt->manifest_count; i++) {
        init_service_id_t id = rt->service_order[i];
        init_service_runtime_t *sr = &rt->services[id];

        if (sr->desc == NULL) continue;

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

        // Mocking synchronous completion for existing simple behavior,
        // to be expanded in async event loop later
        if (err == 0) {
            sr->state = INIT_SERVICE_STATE_READY;
            sr->observed_ready = true;
        } else {
            sr->state = INIT_SERVICE_STATE_FAILED;
        }
    }
}

static bool any_required_failed(const init_runtime_t *rt) {
    for (size_t i = 0; i < rt->manifest_count; i++) {
        init_service_id_t id = rt->service_order[i];
        const init_service_runtime_t *sr = &rt->services[id];
        if (sr->desc == NULL || !sr->required_for_boot) continue;

        if (sr->state == INIT_SERVICE_STATE_FAILED) {
            return true;
        }
    }
    return false;
}

static bool all_required_ready(const init_runtime_t *rt) {
    for (size_t i = 0; i < rt->manifest_count; i++) {
        init_service_id_t id = rt->service_order[i];
        const init_service_runtime_t *sr = &rt->services[id];
        if (sr->desc == NULL || !sr->required_for_boot) continue;

        if (sr->state != INIT_SERVICE_STATE_READY) {
            return false;
        }
    }
    return true;
}

int init_runtime_run(init_boot_context_t *ctx) {
    init_runtime_t rt;
    __builtin_memset(&rt, 0, sizeof(rt));
    rt.boot_ctx = *ctx;

    init_event_queue_init(&rt.event_queue);

    // 1. Filter manifest
    for (size_t i = 0; i < g_init_manifest_count; i++) {
        if (rt.manifest_count >= INIT_SERVICE_ID_MAX) break;

        if (filter_service(&g_init_manifest[i], ctx)) {
            init_service_id_t id = g_init_manifest[i].id;
            rt.service_order[rt.manifest_count] = id;
            rt.services[id].desc = &g_init_manifest[i];
            rt.services[id].state = INIT_SERVICE_STATE_PENDING;
            rt.services[id].attempts = 0;
            rt.services[id].last_error = 0;

            // Map legacy required to boot classes
            if (g_init_manifest[i].policy == INIT_SERVICE_REQUIRED || is_required_boot_class(g_init_manifest[i].boot_class)) {
                rt.services[id].required_for_boot = true;
            }
            rt.manifest_count++;
        }
    }

    // 2. State Machine Loop
    rt.phase = INIT_PHASE_CORE_STARTING;

    bool progress = true;
    while (progress) {
        progress = false; // Detect loop hang if no states advance

        // For each pass, track if we made changes
        for (size_t i = 0; i < rt.manifest_count; i++) {
             init_service_id_t id = rt.service_order[i];
             init_service_state_t old_state = rt.services[id].state;

             // In a real async loop we'd pop events here
        }

        try_issue_bootstrap_hints(&rt);

        // Progress check - if state advanced, keep going
        for (size_t i = 0; i < rt.manifest_count; i++) {
            init_service_id_t id = rt.service_order[i];
            if (rt.services[id].state == INIT_SERVICE_STATE_READY && !rt.services[id].observed_ready) {
                // Was not marked observed ready in previous loop iterations
                progress = true;
            } else if (rt.services[id].state == INIT_SERVICE_STATE_FAILED && rt.services[id].last_error == 0) {
                 progress = true;
            }
        }

        if (any_required_failed(&rt)) {
             rt.failure_class = INIT_FAIL_LAUNCH;
             rt.outcome = INIT_BOOT_OUTCOME_SAFE_MODE;
             ctx->safe_mode_requested = true;
             break;
        }

        if (all_required_ready(&rt)) {
             rt.outcome = INIT_BOOT_OUTCOME_SUCCESS;
             rt.phase = INIT_PHASE_CORE_READY;
             break; // All core done
        }

        // Simplified sync mock progress simulation since stub start returns immediately
        break;
    }

    // Status Report
    init_status_report(rt.services, INIT_SERVICE_ID_MAX);

    // Handoff
    init_handoff_to_supervisor(ctx);

    return (rt.outcome == INIT_BOOT_OUTCOME_SUCCESS) ? 0 : -EFAULT;
}
