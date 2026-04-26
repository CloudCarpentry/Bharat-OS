#include <sched/cpu_partition_validate.h>
#include <stddef.h>

kstatus_t cpu_partition_validate(const bharat_execution_config_t *config) {
    if (!config) {
        return K_ERR_INVALID_ARG;
    }

    if (config->active_cpu_count == 0 || config->active_cpu_count > BHARAT_MAX_CPU_PARTITIONS) {
        return K_ERR_INVALID_ARG;
    }

    bool has_system_role = false;
    bool has_rt_role = false;
    bool has_best_effort_role = false;
    bool has_system_class = false;
    bool has_rt_class = false;
    bool has_fair_class = false;

    for (uint32_t i = 0; i < config->active_cpu_count; i++) {
        const bharat_cpu_partition_desc_t *desc = &config->cpu_partitions[i];

        // 1. All active CPUs assigned a valid role (not NONE)
        if (desc->role == BHARAT_CPU_PARTITION_NONE) {
            return K_ERR_BAD_STATE;
        }

        // 2. All non-SPARE/non-NONE CPUs have at least one sched class
        if (desc->role != BHARAT_CPU_PARTITION_SPARE && desc->allowed_sched_classes == BHARAT_SCHED_CLASS_NONE) {
            return K_ERR_BAD_STATE;
        }

        if (desc->role == BHARAT_CPU_PARTITION_SYSTEM) {
            has_system_role = true;
        }
        if (desc->role == BHARAT_CPU_PARTITION_REALTIME) {
            has_rt_role = true;
        }
        if (desc->role == BHARAT_CPU_PARTITION_BEST_EFFORT) {
            has_best_effort_role = true;
        }

        if (desc->allowed_sched_classes & BHARAT_SCHED_CLASS_SYSTEM) {
            has_system_class = true;
        }
        if (desc->allowed_sched_classes & (BHARAT_SCHED_CLASS_FIFO_RT | BHARAT_SCHED_CLASS_DEADLINE_RT)) {
            has_rt_class = true;
        }
        if (desc->allowed_sched_classes & BHARAT_SCHED_CLASS_FAIR) {
            has_fair_class = true;
        }
    }

    // 3. At least one SYSTEM CPU
    if (!has_system_role) {
        return K_ERR_BAD_STATE;
    }

    // 4. REALTIME mode has RT class when CPU count > 1
    if (config->execution_mode == BHARAT_EXEC_MODE_REALTIME && config->active_cpu_count > 1) {
        if (!has_rt_class) {
            return K_ERR_BAD_STATE;
        }
    }

    // 5. MIXED_CRITICAL mode has SYSTEM + RT + BEST_EFFORT where CPU count allows
    if (config->execution_mode == BHARAT_EXEC_MODE_MIXED_CRITICAL) {
        // Must always have SYSTEM class
        if (!has_system_class) {
            return K_ERR_BAD_STATE;
        }

        if (config->active_cpu_count >= 2) {
            // Should have RT and BEST_EFFORT
            // Note: In 2-core MIXED_CRITICAL, CPU0 has SYSTEM|RT and CPU1 has FAIR.
            if (!has_rt_class || !has_fair_class) {
                return K_ERR_BAD_STATE;
            }
        }
    }

    return K_OK;
}
