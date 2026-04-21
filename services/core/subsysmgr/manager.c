#include "subsys.h"
#include "bharat/subsys_test.h"
#include "hal/hal_cpu_topology.h"
#include "../../../personalities/compat/linux/linux_compat.h"
#include "../../../personalities/compat/windows/win_compat.h"

static uint32_t subsys_allowed_cpu_mask(void) {
    hal_cpu_topology_info_t topology = {0};
    uint32_t discovered = 1U;
    if (hal_cpu_topology_query(&topology) && topology.discovered_cpu_count > 0U) {
        discovered = topology.discovered_cpu_count;
    }
    if (discovered > 32U) {
        discovered = 32U;
    }

    if (discovered == 32U) {
        return UINT32_MAX;
    }
    return (1U << discovered) - 1U;
}

static uint32_t subsys_effective_cpu_mask(uint32_t requested_mask) {
    uint32_t allowed_mask = subsys_allowed_cpu_mask();

    uint32_t effective = requested_mask & allowed_mask;
    if (effective == 0U) {
        effective = 0x1U;
    }

    return effective;
}

static uint32_t next_subsys_id = 1;

int subsys_create(subsys_type_t type, subsys_exec_mode_t mode, subsys_instance_t* out_instance) {
    if (!out_instance) return -1;

    out_instance->subsys_id = next_subsys_id++;
    out_instance->type = type;
    out_instance->exec_mode = mode;
    out_instance->memory_limit_mb = 0;
    out_instance->cpu_core_allocation_mask = subsys_effective_cpu_mask(0U);
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

    instance->cpu_core_allocation_mask =
        subsys_effective_cpu_mask(instance->cpu_core_allocation_mask);

    if (instance->exec_mode == EXECUTION_MODE_TESTING) {
        const char *subsys_name = "unknown";
        switch (instance->type) {
            case SUBSYS_TYPE_LINUX: subsys_name = "linux"; break;
            case SUBSYS_TYPE_WINDOWS: subsys_name = "windows"; break;
            case SUBSYS_TYPE_ANDROID: subsys_name = "android"; break;
            default: break;
        }
        subsys_run_boot_tests(subsys_name);
    }

    instance->is_running = 1;
    return 0;
}

int subsys_destroy(subsys_instance_t* instance) {
    if (!instance) return -1;
    instance->is_running = 0;
    return 0;
}
