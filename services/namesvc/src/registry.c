#include "registry.h"
#include <bharat/runtime/freestanding_string.h>

#define MAX_REGISTRY_ENTRIES 256

typedef struct {
    char name[NAMESVC_MAX_NAME_LEN];
    bharat_cap_handle_t endpoint;
    bool in_use;
} namesvc_entry_t;

static namesvc_entry_t registry[MAX_REGISTRY_ENTRIES];

void namesvc_registry_init(void) {
    memset(registry, 0, sizeof(registry));
}

int32_t namesvc_registry_add(const char *name, bharat_cap_handle_t endpoint) {
    if (!name || name[0] == '\0') {
        return NAMESVC_STATUS_ERR_INVAL;
    }

    if (!bharat_cap_is_valid(endpoint)) {
        return NAMESVC_STATUS_ERR_INVAL;
    }

    // Check if it already exists
    int free_slot = -1;
    for (int i = 0; i < MAX_REGISTRY_ENTRIES; i++) {
        if (registry[i].in_use) {
            // Note: Since `name` might be longer than NAMESVC_MAX_NAME_LEN,
            // we should only compare up to NAMESVC_MAX_NAME_LEN - 1 to match what is
            // actually stored (with the null terminator at the end).
            if (strncmp(registry[i].name, name, NAMESVC_MAX_NAME_LEN - 1) == 0) {
                return NAMESVC_STATUS_ERR_EXISTS;
            }
        } else if (free_slot == -1) {
            free_slot = i;
        }
    }

    if (free_slot == -1) {
        return NAMESVC_STATUS_ERR_NOSPACE;
    }

    strncpy(registry[free_slot].name, name, NAMESVC_MAX_NAME_LEN - 1);
    registry[free_slot].name[NAMESVC_MAX_NAME_LEN - 1] = '\0';
    registry[free_slot].endpoint = endpoint;
    registry[free_slot].in_use = true;

    return NAMESVC_STATUS_OK;
}

int32_t namesvc_registry_lookup(const char *name, bharat_cap_handle_t *endpoint) {
    if (!name || name[0] == '\0' || !endpoint) {
        return NAMESVC_STATUS_ERR_INVAL;
    }

    for (int i = 0; i < MAX_REGISTRY_ENTRIES; i++) {
        if (registry[i].in_use) {
            if (strncmp(registry[i].name, name, NAMESVC_MAX_NAME_LEN - 1) == 0) {
                *endpoint = registry[i].endpoint;
                return NAMESVC_STATUS_OK;
            }
        }
    }

    return NAMESVC_STATUS_ERR_NOTFOUND;
}

int32_t namesvc_registry_remove(const char *name) {
    if (!name || name[0] == '\0') {
        return NAMESVC_STATUS_ERR_INVAL;
    }

    for (int i = 0; i < MAX_REGISTRY_ENTRIES; i++) {
        if (registry[i].in_use) {
            if (strncmp(registry[i].name, name, NAMESVC_MAX_NAME_LEN - 1) == 0) {
                registry[i].in_use = false;
                registry[i].endpoint = BHARAT_CAP_INVALID_HANDLE;
                return NAMESVC_STATUS_OK;
            }
        }
    }

    return NAMESVC_STATUS_ERR_NOTFOUND;
}
