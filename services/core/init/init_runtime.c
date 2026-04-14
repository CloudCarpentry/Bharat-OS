#include "init_runtime.h"
#include "init_manifest.h"
#include "init_status.h"
#include <bharat/runtime/runtime.h>
#include <kernel/status.h>
#include <stddef.h>

static bool is_service_allowed(const init_service_desc_t *desc, const init_boot_context_t *ctx) {
    if (desc->policy == INIT_SERVICE_DISABLED) {
        return false;
    }

    if (!(desc->profile_mask & INIT_PROFILE_MASK(ctx->profile))) {
        return false;
    }

    // Since TINY might not have capabilities or complex structures, these bitwise
    // checks seamlessly fall through if unused, since they are initialized to ~0 or 0
    if ((desc->board_mask & ctx->board_id) != ctx->board_id && ctx->board_id != 0 && desc->board_mask != ~0ULL) {
        return false;
    }

    // Check if required caps are a subset of provided caps
    if ((desc->required_caps & ctx->cap_mask) != desc->required_caps) {
        return false;
    }

    return true;
}

static bool check_dependencies_running(const init_service_desc_t *desc, init_service_runtime_t *runtimes, uint32_t count) {
    if (!desc->deps || desc->dep_count == 0) {
        return true;
    }

    for (uint8_t i = 0; i < desc->dep_count; i++) {
        uint16_t dep_idx = desc->deps[i];
        if (dep_idx >= count) {
            return false;
        }

        if (runtimes[dep_idx].state != INIT_SERVICE_RUNNING) {
            return false;
        }
    }

    return true;
}

int init_runtime_start(const init_boot_context_t *ctx) {
    if (!ctx) return K_ERR_INVALID_ARG;

    init_manifest_t manifest;
    int rc = init_manifest_get(ctx->profile, &manifest);
    if (rc != K_OK) {
        bharat_runtime_log("init_runtime: Failed to load manifest for profile");
        return rc;
    }

    init_service_runtime_t *runtimes = init_status_get_runtimes();

    // Initialize runtimes
    for (uint32_t i = 0; i < manifest.count && i < MAX_INIT_SERVICES; i++) {
        runtimes[i].desc = &manifest.services[i];
        runtimes[i].state = INIT_SERVICE_STOPPED;
        runtimes[i].attempts = 0;
        runtimes[i].last_error = 0;

        if (!is_service_allowed(runtimes[i].desc, ctx)) {
            runtimes[i].state = INIT_SERVICE_SKIPPED;
        }
    }

    // Startup sequence
    // A more advanced engine would do topographical sort, but since the
    // design principles dictate a "deterministic order is explicit" and
    // "bounded dependency check", we assume the manifest is already topologically sorted.
    for (uint32_t i = 0; i < manifest.count && i < MAX_INIT_SERVICES; i++) {
        if (runtimes[i].state == INIT_SERVICE_SKIPPED) {
            continue;
        }

        const init_service_desc_t *desc = runtimes[i].desc;

        // Probe first
        if (desc->probe_fn) {
            int probe_res = desc->probe_fn(NULL);
            if (probe_res != K_OK) {
                runtimes[i].state = INIT_SERVICE_FAILED;
                if (desc->policy == INIT_SERVICE_REQUIRED) {
                    bharat_runtime_log("init_runtime: REQUIRED service probe failed!");
                    return K_ERR_FAULT; // Fail boot
                }
                continue; // Skip starting if optional
            }
        }

        // Check dependencies
        if (!check_dependencies_running(desc, runtimes, manifest.count)) {
            runtimes[i].state = INIT_SERVICE_FAILED;
            if (desc->policy == INIT_SERVICE_REQUIRED) {
                bharat_runtime_log("init_runtime: REQUIRED service dependencies not running!");
                return K_ERR_FAULT;
            }
            continue;
        }

        runtimes[i].state = INIT_SERVICE_STARTING;

#if defined(BHARAT_ENABLE_HOST_TESTS) || defined(BHARAT_BUILD_HOST_TESTS) || !defined(__x86_64__)
        // Let the test harness mock correctly by setting global indices if it defines them
        __attribute__((weak)) extern int g_current_service_idx;
        int *idx_ptr = (int *)&g_current_service_idx;
        if (idx_ptr) {
            *idx_ptr = i;
        }
#endif

        // Start loop with bounded retry
        bool started = false;
        uint8_t limit = desc->retry_limit;
#if defined(BHARAT_INIT_PROFILE_TINY) || defined(BHARAT_INIT_PROFILE_SMALL)
        // Cap retry limit for tiny profiles
        if (limit > 1) {
            limit = 1;
        }
#endif

        while (runtimes[i].attempts <= limit) {
            runtimes[i].attempts++;

            if (desc->start_fn) {
                int start_res = desc->start_fn(NULL);
                if (start_res == K_OK) {
                    started = true;
                    break;
                }
                runtimes[i].last_error = start_res;
            } else {
                // If there's no start fn, assume it's natively integrated or stubbed OK
                started = true;
                break;
            }
        }

        if (started) {
            runtimes[i].state = INIT_SERVICE_RUNNING;
        } else {
            runtimes[i].state = INIT_SERVICE_FAILED;
            if (desc->policy == INIT_SERVICE_REQUIRED) {
                bharat_runtime_log("init_runtime: REQUIRED service failed to start!");
                return K_ERR_FAULT;
            }
        }
    }

    return K_OK;
}
