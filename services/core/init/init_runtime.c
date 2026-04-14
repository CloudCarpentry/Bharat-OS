#include "init_runtime.h"
#include "init_handoff.h"
#include <bharat/runtime/runtime.h>
#include <errno.h>

static bool filter_service(const init_service_desc_t *desc, const init_boot_context_t *ctx) {
    if (!(desc->profile_mask & ctx->profile)) {
        return false;
    }

    // Board mask check
    if (desc->board_mask != BHARAT_INIT_BOARD_ANY && !(desc->board_mask & ctx->board_id)) {
        return false;
    }

    // Personality mask check
    if (desc->personality_mask != BHARAT_INIT_PERSONALITY_ANY && !(desc->personality_mask & ctx->personality_id)) {
        return false;
    }

    // Capability mask check
    if ((desc->required_caps & ctx->cap_mask) != desc->required_caps) {
        return false;
    }

    return true;
}

static init_service_runtime_t* find_runtime_by_id(init_runtime_ctx_t *rt_ctx, init_service_id_t id) {
    for (size_t i = 0; i < rt_ctx->count; i++) {
        if (rt_ctx->services[i].desc->id == id) {
            return &rt_ctx->services[i];
        }
    }
    return NULL;
}

static bool check_dependencies(init_runtime_ctx_t *rt_ctx, const init_service_desc_t *desc) {
    if (desc->dep_count == 0 || desc->deps == NULL) {
        return true;
    }

    for (uint8_t i = 0; i < desc->dep_count; i++) {
        init_service_id_t dep_id = desc->deps[i];
        if (dep_id == INIT_SVC_NONE) continue;

        init_service_runtime_t *dep_rt = find_runtime_by_id(rt_ctx, dep_id);
        if (!dep_rt) {
            bharat_runtime_log("services/init: Missing required dependency");
            return false;
        }

        if (dep_rt->state != INIT_SERVICE_RUNNING) {
            // Not running yet
            return false;
        }
    }

    return true;
}

int init_runtime_run(init_boot_context_t *ctx) {
    init_runtime_ctx_t rt_ctx;
    rt_ctx.count = 0;
    rt_ctx.ctx = ctx;

    // 1. Filter manifest
    for (size_t i = 0; i < g_init_manifest_count; i++) {
        if (rt_ctx.count >= MAX_INIT_SERVICES) break;

        if (filter_service(&g_init_manifest[i], ctx)) {
            rt_ctx.services[rt_ctx.count].desc = &g_init_manifest[i];
            rt_ctx.services[rt_ctx.count].state = INIT_SERVICE_STOPPED;
            rt_ctx.services[rt_ctx.count].attempts = 0;
            rt_ctx.services[rt_ctx.count].last_error = 0;
            rt_ctx.count++;
        }
    }

    // 2. Start services (bounded retries, dependency order)
    bool progress = true;
    while (progress) {
        progress = false;

        for (size_t i = 0; i < rt_ctx.count; i++) {
            init_service_runtime_t *srt = &rt_ctx.services[i];

            if (srt->state == INIT_SERVICE_RUNNING || srt->state == INIT_SERVICE_SKIPPED || srt->state == INIT_SERVICE_FAILED) {
                continue;
            }

            if (!check_dependencies(&rt_ctx, srt->desc)) {
                continue;
            }

            // Try to start
            progress = true; // We are making progress by trying
            srt->state = INIT_SERVICE_STARTING;

#ifdef BHARAT_INIT_PROFILE_TINY
            uint8_t max_retries = srt->desc->retry_limit > 1 ? 1 : srt->desc->retry_limit;
#else
            uint8_t max_retries = srt->desc->retry_limit;
#endif

#ifndef BHARAT_INIT_ENABLE_RETRY
            max_retries = 0;
#endif

            int err = -1;

            if (srt->desc->probe_fn) {
                err = srt->desc->probe_fn(NULL);
                if (err != 0) {
                    srt->state = INIT_SERVICE_SKIPPED;
                    continue;
                }
            }

            while (srt->attempts <= max_retries) {
                srt->attempts++;

                if (srt->desc->start_fn) {
                    err = srt->desc->start_fn(NULL);
                } else {
                    err = 0; // Stub start
                }

                srt->last_error = err;

                if (err == 0) {
                    srt->state = INIT_SERVICE_RUNNING;
                    break;
                }
            }

            if (srt->state != INIT_SERVICE_RUNNING) {
                srt->state = INIT_SERVICE_FAILED;

                if (srt->desc->policy == INIT_SERVICE_REQUIRED) {
                    bharat_runtime_log("services/init: Required service failed to start!");
                    ctx->safe_mode = true;
                    init_status_report(rt_ctx.services, rt_ctx.count);
                    return -EFAULT; // Fail boot
                }
            }
        }
    }

    // 3. Status Report
    init_status_report(rt_ctx.services, rt_ctx.count);

    // 4. Handoff
    init_handoff_to_supervisor(ctx);

    return 0;
}
