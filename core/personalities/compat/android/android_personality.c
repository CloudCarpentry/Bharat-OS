#include "compat/android/android_personality.h"
#include "bh_personality_registry.h"
#include "bh_personality.h"
#include <stddef.h>
#include <stdint.h>
#include "lib/base/string.h"

#define ANDROID_MAX_LOGICAL_OBJS 128

typedef struct {
    android_logical_obj_t obj;
    int is_active;
} android_obj_entry_t;

static android_obj_entry_t g_android_obj_table[ANDROID_MAX_LOGICAL_OBJS];
static int g_android_obj_table_initialized = 0;

static void android_obj_table_init_internal(void) {
    if (!g_android_obj_table_initialized) {
        memset(g_android_obj_table, 0, sizeof(g_android_obj_table));
        /* Root ServiceManager object (ID 0) is always available at home_core 0 */
        g_android_obj_table[0].obj.id = 0;
        g_android_obj_table[0].obj.home_core = 0;
        g_android_obj_table[0].obj.generation = 1;
        g_android_obj_table[0].obj.backing_cap = (cap_handle_t){0};
        g_android_obj_table[0].is_active = 1;
        g_android_obj_table_initialized = 1;
    }
}

static int android_personality_base_init(personality_desc_t* self) {
    if (!self) {
        return -1;
    }

    android_personality_t* p = (android_personality_t*)self;
    p->max_binder_nodes_per_core = 1024;
    p->max_ashmem_regions = 256;

    android_obj_table_init_internal();

    p->service_manager_root.id = 0;
    p->service_manager_root.home_core = 0;
    p->service_manager_root.generation = 1;
    p->service_manager_root.backing_cap = (cap_handle_t){0};

    return 0;
}

int android_personality_register(android_personality_t* p) {
    if (!p) {
        return -1;
    }

    p->base.id = PERS_TYPE_ANDROID;
    p->base.name = "Android";
    p->base.init = android_personality_base_init;
    p->base.start = NULL;
    p->base.stop = NULL;
    p->base.context_data = NULL;

    p->max_binder_nodes_per_core = 1024;
    p->max_ashmem_regions = 256;

    p->service_manager_root.id = 0;
    p->service_manager_root.home_core = 0;
    p->service_manager_root.generation = 1;
    p->service_manager_root.backing_cap = (cap_handle_t){0};

    android_obj_table_init_internal();

    return 0;
}

int android_obj_lookup(android_obj_id_t id, android_logical_obj_t* out_obj) {
    if (!out_obj) {
        return -1;
    }

    android_obj_table_init_internal();

    for (size_t i = 0; i < ANDROID_MAX_LOGICAL_OBJS; i++) {
        if (g_android_obj_table[i].is_active && g_android_obj_table[i].obj.id == id) {
            *out_obj = g_android_obj_table[i].obj;
            return 0;
        }
    }

    /* Object not found or inactive -> fail closed */
    return -1;
}

int android_obj_register(const android_logical_obj_t* obj) {
    if (!obj || obj->generation == 0) {
        return -1;
    }

    android_obj_table_init_internal();

    /* Check if already registered */
    for (size_t i = 0; i < ANDROID_MAX_LOGICAL_OBJS; i++) {
        if (g_android_obj_table[i].is_active && g_android_obj_table[i].obj.id == obj->id) {
            /* Update existing entry */
            g_android_obj_table[i].obj = *obj;
            return 0;
        }
    }

    /* Find free slot */
    for (size_t i = 0; i < ANDROID_MAX_LOGICAL_OBJS; i++) {
        if (!g_android_obj_table[i].is_active) {
            g_android_obj_table[i].obj = *obj;
            g_android_obj_table[i].is_active = 1;
            return 0;
        }
    }

    return -1; /* Table full */
}

int android_obj_unregister(android_obj_id_t id) {
    if (id == 0) {
        return -1; /* Cannot unregister ServiceManager root */
    }

    android_obj_table_init_internal();

    for (size_t i = 0; i < ANDROID_MAX_LOGICAL_OBJS; i++) {
        if (g_android_obj_table[i].is_active && g_android_obj_table[i].obj.id == id) {
            g_android_obj_table[i].is_active = 0;
            memset(&g_android_obj_table[i].obj, 0, sizeof(android_logical_obj_t));
            return 0;
        }
    }

    return -1;
}

#ifndef BHARAT_HOST_TEST
extern const personality_ops_t *personality_linux_get_ops(void);

const personality_ops_t *personality_android_get_ops(void) {
    return personality_linux_get_ops();
}

void android_personality_init(void) {
    bh_personality_registry_register(BH_PERSONALITY_ANDROID, personality_android_get_ops());
}
#endif
