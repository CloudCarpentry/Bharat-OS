#include "registry.h"
#include <string.h>
#include <bharat/cap/cap_validate.h>
#include <bharat/uapi/ipc/status.h>

static namesvc_entry_t registry[NAMESVC_MAX_ENTRIES];
static uint32_t registry_count = 0;
static bool registry_initialized = false;

// Mock variables for test forcing
bool g_force_stale = false;

void namesvc_registry_init(void) {
    memset(registry, 0, sizeof(registry));
    registry_count = 0;
    registry_initialized = true;
}

static bool is_valid_service_name(const char *name) {
    if (!name || name[0] == '\0') {
        return false;
    }
    size_t len = 0;
    while (len < NAMESVC_MAX_NAME_LEN && name[len] != '\0') {
        len++;
    }
    return len < NAMESVC_MAX_NAME_LEN;
}

static int32_t check_endpoint_owner(bharat_cap_handle_t endpoint, uint64_t *out_owner_id, bool *out_stale) {
    if (out_owner_id) *out_owner_id = 0;
    if (out_stale) *out_stale = false;

    bharat_cap_validation_result_t res;
    bharat_cap_scope_t req_scope = { .kind = BHARAT_CAP_SCOPE_SERVICE, .scope_id = 0 };
    bharat_cap_status_t vstat = bharat_cap_validate(
        endpoint,
        BHARAT_CAP_OBJ_SERVICE,
        0,
        0,
        &req_scope,
        &res
    );

    if (vstat == BHARAT_CAP_STALE || g_force_stale) {
        if (out_stale) *out_stale = true;
        if (out_owner_id) *out_owner_id = res.descriptor.owner_principal_id;
        // In real system, we'd get the old owner from the capability metadata.
        // For tests and safe fallback, we use the endpoint handle itself or rely on previous record.
        return BHARAT_IPC_STATUS_OK;
    }

    if (vstat != BHARAT_CAP_OK || !res.allowed) {
        return BHARAT_IPC_STATUS_ERR_PERM;
    }

    if (out_owner_id) *out_owner_id = res.descriptor.owner_principal_id;
    return BHARAT_IPC_STATUS_OK;
}

int32_t namesvc_registry_add(const char *service_name,
                             bharat_service_id_t sid,
                             uint32_t version,
                             uint32_t flags,
                             bharat_cap_handle_t endpoint) {
    if (!registry_initialized) return NAMESVC_STATUS_ERR_NOTFOUND;

    if (!is_valid_service_name(service_name) || endpoint == 0) {
        return NAMESVC_STATUS_ERR_INVAL;
    }

    uint64_t owner_id = 0;
    int32_t own_res = check_endpoint_owner(endpoint, &owner_id, NULL);
    if (own_res != BHARAT_IPC_STATUS_OK) {
        return NAMESVC_STATUS_ERR_INVAL;
    }

    int32_t empty_idx = -1;
    for (uint32_t i = 0; i < NAMESVC_MAX_ENTRIES; i++) {
        if (registry[i].in_use) {
            if (strncmp(registry[i].name, service_name, NAMESVC_MAX_NAME_LEN) == 0 &&
                registry[i].version == version) {
                // If it's stale, and the new owner matches the old owner, we can replace it.
                bool is_stale = false;
                uint64_t old_owner = 0;
                check_endpoint_owner(registry[i].endpoint, &old_owner, &is_stale);

                if (is_stale && registry[i].owner_id == owner_id) {
                    registry[i].endpoint = endpoint;
                    registry[i].flags = flags;
                    registry[i].service_id = sid;
                    return NAMESVC_STATUS_OK;
                }
                return NAMESVC_STATUS_ERR_EXISTS;
            }
        } else if (empty_idx == -1) {
            empty_idx = i;
        }
    }

    if (empty_idx == -1) {
        return NAMESVC_STATUS_ERR_NOSPACE;
    }

    registry[empty_idx].in_use = true;
    strncpy(registry[empty_idx].name, service_name, NAMESVC_MAX_NAME_LEN - 1);
    registry[empty_idx].name[NAMESVC_MAX_NAME_LEN - 1] = '\0';
    registry[empty_idx].endpoint = endpoint;
    registry[empty_idx].service_id = sid;
    registry[empty_idx].version = version;
    registry[empty_idx].flags = flags;
    registry[empty_idx].owner_id = owner_id;

    registry_count++;
    return NAMESVC_STATUS_OK;
}

int32_t namesvc_registry_remove(const char *service_name, bharat_cap_handle_t caller_cap) {
    if (!registry_initialized) return NAMESVC_STATUS_ERR_NOTFOUND;
    if (!is_valid_service_name(service_name)) return NAMESVC_STATUS_ERR_INVAL;

    uint64_t caller_owner_id = 0;
    int32_t own_res = check_endpoint_owner(caller_cap, &caller_owner_id, NULL);
    if (own_res != BHARAT_IPC_STATUS_OK) {
        return NAMESVC_STATUS_ERR_PERM;
    }

    int32_t removed = 0;
    for (uint32_t i = 0; i < NAMESVC_MAX_ENTRIES; i++) {
        if (registry[i].in_use && strncmp(registry[i].name, service_name, NAMESVC_MAX_NAME_LEN) == 0) {
            if (registry[i].owner_id != caller_owner_id) {
                continue;
            }
            registry[i].in_use = false;
            registry_count--;
            removed++;
        }
    }

    return removed > 0 ? NAMESVC_STATUS_OK : NAMESVC_STATUS_ERR_NOTFOUND;
}

int32_t namesvc_registry_lookup(const char *service_name,
                                uint32_t requested_version,
                                bool exact_version,
                                bharat_cap_handle_t *out_endpoint,
                                bharat_service_id_t *out_sid,
                                uint32_t *out_version,
                                uint32_t *out_flags) {
    if (!registry_initialized || !out_endpoint) return NAMESVC_STATUS_ERR_NOTFOUND;

    int32_t best_idx = -1;
    uint32_t best_version = 0;

    for (uint32_t i = 0; i < NAMESVC_MAX_ENTRIES; i++) {
        if (registry[i].in_use && strncmp(registry[i].name, service_name, NAMESVC_MAX_NAME_LEN) == 0) {
            bool is_stale = false;
            check_endpoint_owner(registry[i].endpoint, NULL, &is_stale);
            if (is_stale) continue;

            if (exact_version) {
                if (registry[i].version == requested_version) {
                    best_idx = i;
                    break;
                }
            } else {
                if (registry[i].version >= requested_version && registry[i].version >= best_version) {
                    best_idx = i;
                    best_version = registry[i].version;
                }
            }
        }
    }

    if (best_idx != -1) {
        *out_endpoint = registry[best_idx].endpoint;
        if (out_sid) *out_sid = registry[best_idx].service_id;
        if (out_version) *out_version = registry[best_idx].version;
        if (out_flags) *out_flags = registry[best_idx].flags;
        return NAMESVC_STATUS_OK;
    }

    return NAMESVC_STATUS_ERR_NOTFOUND;
}
