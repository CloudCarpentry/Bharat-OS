#include "subsys.h"
#include "linux_compat.h"
#include "win_compat.h"

static uint32_t next_subsys_id = 1;

int subsys_create(subsys_type_t type, subsys_exec_mode_t mode, subsys_instance_t* out_instance) {
    if (!out_instance) return -1;

    out_instance->subsys_id = next_subsys_id++;
    out_instance->type = type;
    out_instance->exec_mode = mode;
    out_instance->memory_limit_mb = 0;
    out_instance->cpu_core_allocation_mask = 0;
    out_instance->is_running = 0;

    switch (type) {
        case SUBSYS_TYPE_LINUX:
            return linux_subsys_init(out_instance);
        case SUBSYS_TYPE_WINDOWS:
            return winnt_subsys_init(out_instance);
        default:
            return 0;
    }
}

int subsys_load_env(subsys_instance_t* instance, const char* root_path) {
    if (!instance || !root_path) return -1;
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
