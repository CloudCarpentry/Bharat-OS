#include <sched/sched_admission.h>
#include <sched/cpu_partition.h>
#include <profile/execution_mode.h>
#include <stddef.h>

kstatus_t sched_admission_allows_class(
    uint32_t cpu_id,
    bharat_sched_class_mask_t class_mask
) {
    if (cpu_partition_allows_class(cpu_id, class_mask)) {
        return K_OK;
    }
    return K_ERR_DENIED;
}

kstatus_t sched_admission_select_cpu(
    bharat_sched_class_mask_t class_mask,
    uint32_t *out_cpu_id
) {
    const bharat_execution_config_t *cfg = bharat_execution_mode_get_config();
    if (!cfg || !out_cpu_id) {
        return K_ERR_INVALID_ARG;
    }

    /*
     * SCHED1 selection is deterministic and intentionally simple:
     * choose the first eligible CPU. Load-aware selection belongs to a
     * later scheduler-balancing phase.
     */
    for (uint32_t i = 0; i < cfg->active_cpu_count; i++) {
        if (cpu_partition_allows_class(i, class_mask)) {
            *out_cpu_id = i;
            return K_OK;
        }
    }

    return K_ERR_NOT_FOUND;
}

kstatus_t sched_admission_validate_partitions(void) {
    const bharat_execution_config_t *cfg = bharat_execution_mode_get_config();
    if (!cfg) {
        return K_ERR_BAD_STATE;
    }

    if (cfg->active_cpu_count == 0) {
        return K_ERR_INVALID_ARG;
    }

    if (cfg->active_cpu_count > BHARAT_MAX_CPU_PARTITIONS) {
        return K_ERR_OVERFLOW;
    }

    if (cfg->execution_mode == BHARAT_EXEC_MODE_UNKNOWN) {
        return K_ERR_BAD_STATE;
    }

    // 1. One SYSTEM CPU must exist
    bool has_system = false;
    for (uint32_t i = 0; i < cfg->active_cpu_count; i++) {
        if (cfg->cpu_partitions[i].role == BHARAT_CPU_PARTITION_SYSTEM) {
            has_system = true;
            break;
        }
    }
    if (!has_system) {
        return K_ERR_DENIED;
    }

    // 2. Single-core mode must use TEMPORAL strategy
    if (cfg->active_cpu_count == 1) {
        if (cfg->partition_strategy != BHARAT_PARTITION_STRATEGY_TEMPORAL) {
            return K_ERR_BAD_STATE;
        }
    } else {
        // 3. Multi-core mode should use SPATIAL strategy
        if (cfg->partition_strategy != BHARAT_PARTITION_STRATEGY_SPATIAL) {
            return K_ERR_BAD_STATE;
        }

        // 4. MIX mode must not collapse RT and FAIR on multi-core
        if (cfg->execution_mode == BHARAT_EXEC_MODE_MIXED_CRITICAL) {
            bool has_rt = false;
            bool has_fair = false;
            for (uint32_t i = 0; i < cfg->active_cpu_count; i++) {
                uint32_t mask = cfg->cpu_partitions[i].allowed_sched_classes;
                if (mask & BHARAT_SCHED_CLASS_FIFO_RT) has_rt = true;
                if (mask & BHARAT_SCHED_CLASS_FAIR) has_fair = true;

                if ((mask & BHARAT_SCHED_CLASS_FIFO_RT) && (mask & BHARAT_SCHED_CLASS_FAIR)) {
                    return K_ERR_BAD_STATE;
                }
            }
            if (!has_rt || !has_fair) {
                return K_ERR_BAD_STATE;
            }
        }

        // 5. RT mode must have at least one RT-capable CPU
        if (cfg->execution_mode == BHARAT_EXEC_MODE_REALTIME) {
            bool has_rt = false;
            for (uint32_t i = 0; i < cfg->active_cpu_count; i++) {
                if (cfg->cpu_partitions[i].allowed_sched_classes & BHARAT_SCHED_CLASS_FIFO_RT) {
                    has_rt = true;
                    break;
                }
            }
            if (!has_rt) {
                return K_ERR_BAD_STATE;
            }
        }
    }

    return K_OK;
}

const char *sched_admission_mode_to_string(bharat_execution_mode_t mode) {
    switch (mode) {
        case BHARAT_EXEC_MODE_REALTIME:
            return "RT";
        case BHARAT_EXEC_MODE_GENERAL_PURPOSE:
            return "GP";
        case BHARAT_EXEC_MODE_MIXED_CRITICAL:
            return "MIX";
        case BHARAT_EXEC_MODE_UNKNOWN:
        default:
            return "UNKNOWN";
    }
}

const char *sched_admission_partition_to_string(bharat_partition_strategy_t strategy) {
    switch (strategy) {
        case BHARAT_PARTITION_STRATEGY_SPATIAL:
            return "SPATIAL";
        case BHARAT_PARTITION_STRATEGY_TEMPORAL:
            return "TEMPORAL";
        case BHARAT_PARTITION_STRATEGY_NONE:
        default:
            return "UNKNOWN";
    }
}
