#include "subsys.h"
#include "bharat/subsys_test.h"
#include "hal/hal_cpu_topology.h"
#include "../../../personalities/compat/linux/linux_compat.h"
#include "../../../personalities/compat/windows/win_compat.h"

uint32_t subsys_resolve_effective_cpu_mask(const hal_cpu_topology_info_t* topology, const subsys_alloc_config_t* config, uint32_t requested_mask) {
    if (!topology || !config) return 1U; // Safe fallback

    // 1. Determine requested mask
    uint32_t base_request = requested_mask;
    if (base_request == 0U) {
        base_request = config->preferred_cpu_mask;
        if (base_request == 0U) {
            base_request = topology->valid_cpu_mask;
        }
    }

    // 2. Sanitize against valid mask
    uint32_t eligible_mask = base_request & topology->valid_cpu_mask;
    if (eligible_mask == 0U) {
        eligible_mask = topology->valid_cpu_mask;
    }

    // 3. Apply cluster preference
    if (config->cluster_pref == SUBSYS_CLUSTER_PREF_PERF) {
        if (topology->performance_cluster_mask != 0U) {
            uint32_t pref_mask = eligible_mask & topology->performance_cluster_mask;
            if (pref_mask != 0U) eligible_mask = pref_mask;
        }
    } else if (config->cluster_pref == SUBSYS_CLUSTER_PREF_EFF) {
        if (topology->efficiency_cluster_mask != 0U) {
            uint32_t pref_mask = eligible_mask & topology->efficiency_cluster_mask;
            if (pref_mask != 0U) eligible_mask = pref_mask;
        }
    }

    // 4. Apply strategy (PACKED vs SPREAD)
    if (eligible_mask != 0U && (eligible_mask & (eligible_mask - 1)) != 0U) { // Multiple CPUs eligible
        if (config->policy == SUBSYS_ALLOC_PACKED) {
            // Pick lowest valid CPU
            uint32_t lowest_bit = eligible_mask & (~eligible_mask + 1);
            eligible_mask = lowest_bit;
        } else if (config->policy == SUBSYS_ALLOC_SPREAD) {
            // Keep all eligible CPUs to spread load
            // (In a more advanced system, we might pick alternating bits, but for now, spread means use all eligible)
        }
    }

    if (eligible_mask == 0U) return 1U; // Safe fallback

    return eligible_mask;
}

static uint32_t subsys_effective_cpu_mask(const subsys_alloc_config_t* config, uint32_t requested_mask) {
    hal_cpu_topology_info_t topology = {0};
    topology.discovered_cpu_count = 1U;
    topology.valid_cpu_mask = 1U;
    topology.performance_cluster_mask = 1U;
    topology.efficiency_cluster_mask = 0U;
    topology.smp_available = false;
    topology.homogeneous_cores = true;

    hal_cpu_topology_query(&topology);

    return subsys_resolve_effective_cpu_mask(&topology, config, requested_mask);
}

static uint32_t next_subsys_id = 1;

int subsys_create(subsys_type_t type, subsys_exec_mode_t mode, const subsys_alloc_config_t* alloc_cfg, subsys_instance_t* out_instance) {
    if (!out_instance) return -1;

    out_instance->subsys_id = next_subsys_id++;
    out_instance->type = type;
    out_instance->exec_mode = mode;
    out_instance->memory_limit_mb = 0;

    if (alloc_cfg) {
        out_instance->alloc_config = *alloc_cfg;
    } else {
        out_instance->alloc_config.policy = SUBSYS_ALLOC_DEFAULT;
        out_instance->alloc_config.preferred_cpu_mask = 0U;
        out_instance->alloc_config.cluster_pref = SUBSYS_CLUSTER_PREF_NONE;
    }

    out_instance->cpu_core_allocation_mask = subsys_effective_cpu_mask(&out_instance->alloc_config, 0U);
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
        subsys_effective_cpu_mask(&instance->alloc_config, instance->cpu_core_allocation_mask);

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
