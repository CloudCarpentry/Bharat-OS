#include "fs/object_store.h"

#define MAX_OBJECT_STORES 4

static object_store_t* g_object_stores[MAX_OBJECT_STORES];
static size_t g_object_store_count = 0;

int object_store_register(object_store_t* store, capability_t* cap) {
    if (!store || !cap) {
        return -1;
    }

    if (g_object_store_count >= MAX_OBJECT_STORES) {
        return -1;
    }

    g_object_stores[g_object_store_count++] = store;
    return 0;
}

int object_store_lookup(const char* uri, object_store_t** out_store, capability_t* cap) {
    if (!uri || !out_store || !cap) {
        return -1;
    }

    // TODO: Verify capability allows store access

    // Iterate through stores, returning match based on URI prefix
    for (size_t i = 0; i < g_object_store_count; i++) {
        const char *store_uri = g_object_stores[i]->uri;
        size_t j = 0;
        int match = 1;

        while (store_uri[j] != '\0' && uri[j] != '\0') {
            if (store_uri[j] != uri[j]) {
                match = 0;
                break;
            }
            j++;
        }

        if (match && store_uri[j] == '\0' && (uri[j] == '\0' || uri[j] == '/')) {
            *out_store = g_object_stores[i];
            return 0;
        }
    }

    *out_store = NULL;
    return -1;
}
