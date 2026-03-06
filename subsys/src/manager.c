#include "subsys.h"

// Basic stub implementation for the subsystem manager
int subsys_create(subsys_type_t type, subsys_exec_mode_t mode, subsys_instance_t* out_instance) {
    if (!out_instance) return -1;
    
    out_instance->subsys_id = 1; // dummy ID
    out_instance->type = type;
    out_instance->exec_mode = mode;
    out_instance->is_running = 0;
    
    return 0; // Success
}

int subsys_load_env(subsys_instance_t* instance, const char* root_path) {
    if (!instance) return -1;
    // Load rootfs...
    return 0;
}

int subsys_start(subsys_instance_t* instance) {
    if (!instance) return -1;
    instance->is_running = 1;
    return 0;
}

int subsys_destroy(subsys_instance_t* instance) {
    if (!instance) return -1;
    instance->is_running = 0;
    return 0;
}
