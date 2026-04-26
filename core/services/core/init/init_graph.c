#include "init_graph.h"
#include <bharat/runtime/runtime.h>
#include <stdbool.h>

init_failure_class_t init_graph_failure_to_init_failure(init_graph_result_t rc) {
    switch (rc) {
        case INIT_GRAPH_ERR_UNKNOWN_DEP:
        case INIT_GRAPH_ERR_FILTERED_DEP:
        case INIT_GRAPH_ERR_CYCLE:
        case INIT_GRAPH_ERR_DUPLICATE_SERVICE:
            return INIT_FAIL_DEP;

        case INIT_GRAPH_ERR_NO_CORE_SERVICE:
        case INIT_GRAPH_ERR_REQUIRED_CAP_MISSING:
            return INIT_FAIL_PROFILE;

        case INIT_GRAPH_OK:
        default:
            return INIT_FAIL_NONE;
    }
}

static const init_service_desc_t* find_service_by_id(const init_service_desc_t *manifest,
                                                     size_t count,
                                                     init_service_id_t id) {
    for (size_t i = 0; i < count; i++) {
        if (manifest[i].id == id) {
            return &manifest[i];
        }
    }
    return NULL;
}

static bool check_cycle_recursive(init_service_id_t id,
                                  const init_service_desc_t *manifest,
                                  size_t count,
                                  bool *visited,
                                  bool *rec_stack) {
    if (id == INIT_SVC_NONE) return false;

    // Use a simple mapping if IDs are small, or just find index
    size_t idx = 0;
    bool found = false;
    for (size_t i = 0; i < count; i++) {
        if (manifest[i].id == id) {
            idx = i;
            found = true;
            break;
        }
    }
    if (!found) return false; // Should not happen if pre-validated

    if (!visited[idx]) {
        visited[idx] = true;
        rec_stack[idx] = true;

        const init_service_desc_t *svc = &manifest[idx];
        for (uint8_t i = 0; i < svc->dep_count; i++) {
            if (check_cycle_recursive(svc->deps[i], manifest, count, visited, rec_stack)) {
                return true;
            }
        }
    } else if (rec_stack[idx]) {
        return true;
    }

    rec_stack[idx] = false;
    return false;
}

init_graph_result_t init_graph_validate(const init_service_desc_t *manifest,
                                        size_t count,
                                        const init_boot_context_t *ctx) {
    if (count == 0) return INIT_GRAPH_ERR_NO_CORE_SERVICE;

    bool has_core = false;
    for (size_t i = 0; i < count; i++) {
        const init_service_desc_t *svc = &manifest[i];

        // 1. Check duplicate IDs
        for (size_t j = i + 1; j < count; j++) {
            if (svc->id == manifest[j].id) {
                return INIT_GRAPH_ERR_DUPLICATE_SERVICE;
            }
        }

        // 2. Track CORE presence
        if (svc->boot_class == BOOT_CLASS_CORE) {
            has_core = true;
        }

        // 3. Check capabilities for required services
        if (svc->policy == INIT_SERVICE_REQUIRED || svc->boot_class == BOOT_CLASS_CORE || svc->boot_class == BOOT_CLASS_INFRA) {
             if ((svc->required_caps & ctx->capability_mask) != svc->required_caps) {
                 return INIT_GRAPH_ERR_REQUIRED_CAP_MISSING;
             }
        }

        // 4. Check dependencies
        for (uint8_t d = 0; d < svc->dep_count; d++) {
            init_service_id_t dep_id = svc->deps[d];
            if (dep_id == INIT_SVC_NONE) continue;

            const init_service_desc_t *dep = find_service_by_id(manifest, count, dep_id);
            if (!dep) {
                // Dependency missing in filtered list.
                // If it's in g_init_manifest, then it was filtered out.
                bool filtered = false;
                for (size_t k = 0; k < g_init_manifest_count; k++) {
                    if (g_init_manifest[k].id == dep_id) {
                        filtered = true;
                        break;
                    }
                }
                if (filtered) {
                    return INIT_GRAPH_ERR_FILTERED_DEP;
                } else {
                    return INIT_GRAPH_ERR_UNKNOWN_DEP;
                }
            }
        }
    }

    if (!has_core) return INIT_GRAPH_ERR_NO_CORE_SERVICE;

    // 5. Cycle Detection
    bool visited[INIT_SERVICE_ID_MAX] = {0};
    bool rec_stack[INIT_SERVICE_ID_MAX] = {0};

    for (size_t i = 0; i < count; i++) {
        if (check_cycle_recursive(manifest[i].id, manifest, count, visited, rec_stack)) {
            return INIT_GRAPH_ERR_CYCLE;
        }
    }

    return INIT_GRAPH_OK;
}
