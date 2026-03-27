#include "bharat/stacks/storage/fs/adapter.h"

#define MAX_ADAPTERS 8

static fs_adapter_t* g_adapters[MAX_ADAPTERS];
static size_t g_adapter_count = 0;

int fs_adapter_register(fs_adapter_t* adapter) {
    if (!adapter || !adapter->name) {
        return -1;
    }
    if (g_adapter_count >= MAX_ADAPTERS) {
        return -1;
    }
    g_adapters[g_adapter_count++] = adapter;
    return 0;
}

static int fs_streq(const char *a, const char *b) {
    size_t i = 0;
    if (!a || !b) return 0;
    while (a[i] != '\0' && b[i] != '\0') {
        if (a[i] != b[i]) return 0;
        i++;
    }
    return a[i] == b[i];
}

const fs_adapter_t* fs_adapter_get(const char* name) {
    if (!name) return NULL;
    for (size_t i = 0; i < g_adapter_count; i++) {
        if (fs_streq(g_adapters[i]->name, name)) {
            return g_adapters[i];
        }
    }
    return NULL;
}
