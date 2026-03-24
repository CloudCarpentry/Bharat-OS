#include "registry.h"
#include <bharat/runtime/freestanding_string.h>

#define MAX_REGISTRY_ENTRIES 256

typedef struct {
    char service_name[NAMESVC_MAX_NAME_LEN];
    char interface_name[NAMESVC_MAX_NAME_LEN];
    uint32_t interface_version;
    uint32_t transport_flags;
    bharat_cap_handle_t endpoint;
    bool in_use;
} namesvc_entry_t;

static namesvc_entry_t registry[MAX_REGISTRY_ENTRIES];

void namesvc_registry_init(void) {
    memset(registry, 0, sizeof(registry));
}

int32_t namesvc_registry_add(const char *service_name,
                             const char *interface_name,
                             uint32_t interface_version,
                             uint32_t transport_flags,
                             bharat_cap_handle_t endpoint) {
    if (!service_name || service_name[0] == '\0' || !interface_name || interface_name[0] == '\0') {
        return NAMESVC_STATUS_ERR_INVAL;
    }

    if (!bharat_cap_is_valid(endpoint)) {
        return NAMESVC_STATUS_ERR_INVAL;
    }

    int free_slot = -1;
    for (int i = 0; i < MAX_REGISTRY_ENTRIES; i++) {
        if (registry[i].in_use) {
            if (strncmp(registry[i].service_name, service_name, NAMESVC_MAX_NAME_LEN - 1) == 0 &&
                strncmp(registry[i].interface_name, interface_name, NAMESVC_MAX_NAME_LEN - 1) == 0) {

                // If it's the exact same version, reject as exists/conflict
                if (registry[i].interface_version == interface_version) {
                    return NAMESVC_STATUS_ERR_EXISTS;
                }
            }
        } else if (free_slot == -1) {
            free_slot = i;
        }
    }

    if (free_slot == -1) {
        return NAMESVC_STATUS_ERR_NOSPACE;
    }

    strncpy(registry[free_slot].service_name, service_name, NAMESVC_MAX_NAME_LEN - 1);
    registry[free_slot].service_name[NAMESVC_MAX_NAME_LEN - 1] = '\0';

    strncpy(registry[free_slot].interface_name, interface_name, NAMESVC_MAX_NAME_LEN - 1);
    registry[free_slot].interface_name[NAMESVC_MAX_NAME_LEN - 1] = '\0';

    registry[free_slot].interface_version = interface_version;
    registry[free_slot].transport_flags = transport_flags;
    registry[free_slot].endpoint = endpoint;
    registry[free_slot].in_use = true;

    return NAMESVC_STATUS_OK;
}

int32_t namesvc_registry_lookup(const char *service_name,
                                const char *interface_name,
                                uint32_t requested_version,
                                bool exact_version,
                                bharat_cap_handle_t *endpoint,
                                uint32_t *out_version,
                                uint32_t *out_transport_flags) {
    if (!service_name || service_name[0] == '\0' || !interface_name || interface_name[0] == '\0' || !endpoint) {
        return NAMESVC_STATUS_ERR_INVAL;
    }

    int best_match_idx = -1;
    uint32_t best_version = 0;

    for (int i = 0; i < MAX_REGISTRY_ENTRIES; i++) {
        if (registry[i].in_use) {
            if (strncmp(registry[i].service_name, service_name, NAMESVC_MAX_NAME_LEN - 1) == 0 &&
                strncmp(registry[i].interface_name, interface_name, NAMESVC_MAX_NAME_LEN - 1) == 0) {

                if (exact_version) {
                    if (registry[i].interface_version == requested_version) {
                        best_match_idx = i;
                        break;
                    }
                } else {
                    // For compatible version, assume higher version >= requested version is backwards compatible.
                    // Or for now, just find the highest version >= requested_version
                    if (registry[i].interface_version >= requested_version) {
                        if (best_match_idx == -1 || registry[i].interface_version > best_version) {
                            best_match_idx = i;
                            best_version = registry[i].interface_version;
                        }
                    }
                }
            }
        }
    }

    if (best_match_idx != -1) {
        *endpoint = registry[best_match_idx].endpoint;
        if (out_version) *out_version = registry[best_match_idx].interface_version;
        if (out_transport_flags) *out_transport_flags = registry[best_match_idx].transport_flags;
        return NAMESVC_STATUS_OK;
    }

    return NAMESVC_STATUS_ERR_NOTFOUND;
}

int32_t namesvc_registry_remove(const char *service_name,
                                const char *interface_name,
                                uint32_t interface_version) {
    if (!service_name || service_name[0] == '\0' || !interface_name || interface_name[0] == '\0') {
        return NAMESVC_STATUS_ERR_INVAL;
    }

    for (int i = 0; i < MAX_REGISTRY_ENTRIES; i++) {
        if (registry[i].in_use) {
            if (strncmp(registry[i].service_name, service_name, NAMESVC_MAX_NAME_LEN - 1) == 0 &&
                strncmp(registry[i].interface_name, interface_name, NAMESVC_MAX_NAME_LEN - 1) == 0 &&
                registry[i].interface_version == interface_version) {

                registry[i].in_use = false;
                registry[i].endpoint = BHARAT_CAP_INVALID_HANDLE;
                return NAMESVC_STATUS_OK;
            }
        }
    }

    return NAMESVC_STATUS_ERR_NOTFOUND;
}
