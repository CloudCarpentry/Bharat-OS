#include "compat/android/android_personality.h"
#include "personality/personality_types.h"
#include <string.h>

static int android_init_callback(personality_desc_t* self) {
    if (!self) return -1;
    android_personality_t* p = (android_personality_t*)self->context_data;
    if (!p) return -1;

    // Phase 0: Basic initialization
    p->max_binder_nodes_per_core = 1024;
    p->max_ashmem_regions = 256;
    
    // Initialize the root service manager ID
    p->service_manager_root.id = 0;
    p->service_manager_root.home_core = 0; // Default to BSP
    p->service_manager_root.generation = 1;

    return 0;
}

static int android_start_callback(personality_desc_t* self) {
    (void)self;
    // Phase 0: Transition to running state
    return 0;
}

static int android_stop_callback(personality_desc_t* self) {
    (void)self;
    // Phase 0: Cleanup
    return 0;
}

int android_personality_register(android_personality_t* p) {
    if (!p) return -1;

    // Fill the base personality descriptor
    p->base.id = PERS_TYPE_ANDROID;
    p->base.name = "Android";
    p->base.init = android_init_callback;
    p->base.start = android_start_callback;
    p->base.stop = android_stop_callback;
    p->base.context_data = p;

    // In a real system, this would call a core kernel registration function:
    // kernel_register_personality(&p->base);
    
    return 0;
}

int android_obj_lookup(android_obj_id_t id, android_logical_obj_t* out_obj) {
    if (!out_obj) return -1;
    
    // Phase 0: Mock lookup logic.
    // In Phase 1-2, this will query the distributed capability map.
    if (id == 0) {
        out_obj->id = 0;
        out_obj->home_core = 0;
        out_obj->generation = 1;
        out_obj->backing_cap = (cap_handle_t){0}; // Proper struct initialization
        return 0;
    }

    return -1; // Not found
}
