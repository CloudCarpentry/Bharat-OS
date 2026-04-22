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
    uint32_t valid_mask = topology->valid_cpu_mask;
    if (valid_mask == 0U) {
        valid_mask = mask_for_cpu_count(discovered);
    }

    if (valid_mask == 0U) {
        return 0U;
    }

    subsys_alloc_config_t effective_cfg = config ? *config : subsys_default_alloc_config();
    uint32_t sanitized_requested = requested_mask & valid_mask;
    uint32_t target_count = popcount32(sanitized_requested);
    if (target_count == 0U) {
        target_count = 1U;
    }

    uint32_t eligible_mask = sanitized_requested;
    if (eligible_mask == 0U) {
        eligible_mask = valid_mask;
    }

    if (effective_cfg.cluster_preference == SUBSYS_CLUSTER_PREFERENCE_PERFORMANCE) {
        uint32_t perf_mask = topology->performance_cluster_mask & valid_mask;
        if (perf_mask != 0U) {
            uint32_t preferred = eligible_mask & perf_mask;
            if (preferred != 0U) {
                eligible_mask = preferred;
            }
        }
    } else if (effective_cfg.cluster_preference == SUBSYS_CLUSTER_PREFERENCE_EFFICIENCY) {
        uint32_t eff_mask = topology->efficiency_cluster_mask & valid_mask;
        if (eff_mask != 0U) {
            uint32_t preferred = eligible_mask & eff_mask;
            if (preferred != 0U) {
                eligible_mask = preferred;
            }
        }
    }

    uint32_t eligible_count = popcount32(eligible_mask);
    if (target_count > eligible_count) {
        target_count = eligible_count;
    }

    switch (effective_cfg.policy) {
        case SUBSYS_ALLOC_SPREAD:
            return pick_spread_n(eligible_mask, target_count);
        case SUBSYS_ALLOC_DEFAULT:
        case SUBSYS_ALLOC_PACKED:
        default:
            return pick_lowest_n(eligible_mask, target_count);
    }
}

static uint32_t subsys_effective_cpu_mask(uint32_t requested_mask, const subsys_alloc_config_t* cfg) {
    hal_cpu_topology_info_t topology = {0};
    if (!hal_cpu_topology_query(&topology)) {
        return subsys_resolve_effective_cpu_mask(NULL, requested_mask, cfg);
    }
    return subsys_resolve_effective_cpu_mask(&topology, requested_mask, cfg);
}

static uint32_t next_subsys_id = 1;

int subsys_create_with_config(subsys_type_t type, subsys_exec_mode_t mode, const subsys_alloc_config_t* cfg, subsys_instance_t* out_instance) {
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

int subsys_create(subsys_type_t type, subsys_exec_mode_t mode, subsys_instance_t* out_instance) {
    return subsys_create_with_config(type, mode, NULL, out_instance);
}

int subsys_set_alloc_config(subsys_instance_t* instance, const subsys_alloc_config_t* cfg) {
    if (!instance || !cfg) {
        return -1;
    }
    if (instance->is_running) {
        return -2;
    }

    instance->alloc_config = *cfg;
    instance->requested_cpu_mask = cfg->preferred_cpu_mask;
    instance->cpu_core_allocation_mask =
        subsys_effective_cpu_mask(instance->requested_cpu_mask, &instance->alloc_config);
    return 0;
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
