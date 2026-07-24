#include <sched/cpu_partition.h>
#include <sched/cpu_partition_validate.h>
#include <profile/execution_mode.h>
#include <stddef.h>

/**
 * @brief Initialize CPU partitioning based on execution config.
 *
 * NOTE: This implementation follows a deterministic default bootstrap policy.
 * Future work should consume HAL/platform topology hints (e.g. big.LITTLE, NUMA)
 * to refine core roles.
 */
kstatus_t cpu_partition_init(bharat_execution_config_t *config) {
    if (!config) {
        return K_ERR_INVALID_ARG;
    }

    if (config->active_cpu_count == 0 || config->active_cpu_count > BHARAT_MAX_CPU_PARTITIONS) {
        return K_ERR_INVALID_ARG;
    }

    // Step 1: Strategy Resolution
    if (config->active_cpu_count <= 1) {
        config->partition_strategy = BHARAT_PARTITION_STRATEGY_TEMPORAL;
    } else {
        config->partition_strategy = BHARAT_PARTITION_STRATEGY_SPATIAL;
    }

    // Clear old mappings and initialize CPU IDs
    for (uint32_t i = 0; i < BHARAT_MAX_CPU_PARTITIONS; i++) {
        config->cpu_partitions[i].cpu_id = i;
        config->cpu_partitions[i].role = BHARAT_CPU_PARTITION_NONE;
        config->cpu_partitions[i].allowed_sched_classes = BHARAT_SCHED_CLASS_NONE;
        config->cpu_partitions[i].housekeeping = false;
        config->cpu_partitions[i].allow_migration_in = true;
        config->cpu_partitions[i].allow_migration_out = true;
        config->cpu_partitions[i].irq_preferred = false;
    }

    // Step 2: Implement Default Mappings
    if (config->active_cpu_count == 1) {
        // Temporal mapping: everything coexistence
        config->cpu_partitions[0].role = BHARAT_CPU_PARTITION_SYSTEM;
        config->cpu_partitions[0].allowed_sched_classes =
            BHARAT_SCHED_CLASS_SYSTEM |
            BHARAT_SCHED_CLASS_FIFO_RT |
            BHARAT_SCHED_CLASS_DEADLINE_RT |
            BHARAT_SCHED_CLASS_FAIR;
    } else if (config->active_cpu_count == 2) {
        // Compressed mapping
        if (config->execution_mode == BHARAT_EXEC_MODE_MIXED_CRITICAL ||
            config->execution_mode == BHARAT_EXEC_MODE_REALTIME) {

            // CPU0: SYSTEM + RT
            config->cpu_partitions[0].role = BHARAT_CPU_PARTITION_SYSTEM;
            config->cpu_partitions[0].allowed_sched_classes = BHARAT_SCHED_CLASS_SYSTEM | BHARAT_SCHED_CLASS_FIFO_RT | BHARAT_SCHED_CLASS_DEADLINE_RT;

            // CPU1: BEST EFFORT (or SPARE for pure realtime)
            if (config->execution_mode == BHARAT_EXEC_MODE_MIXED_CRITICAL) {
                config->cpu_partitions[1].role = BHARAT_CPU_PARTITION_BEST_EFFORT;
                config->cpu_partitions[1].allowed_sched_classes = BHARAT_SCHED_CLASS_FAIR;
            } else {
                config->cpu_partitions[1].role = BHARAT_CPU_PARTITION_SPARE;
                config->cpu_partitions[1].allowed_sched_classes = BHARAT_SCHED_CLASS_NONE;
            }
        } else {
            // General Purpose
            for (int i = 0; i < 2; i++) {
                config->cpu_partitions[i].role = BHARAT_CPU_PARTITION_SYSTEM;
                config->cpu_partitions[i].allowed_sched_classes = BHARAT_SCHED_CLASS_SYSTEM | BHARAT_SCHED_CLASS_FAIR;
            }
        }
    } else if (config->active_cpu_count == 3) {
        if (config->execution_mode == BHARAT_EXEC_MODE_GENERAL_PURPOSE) {
            for (int i = 0; i < 3; i++) {
                config->cpu_partitions[i].role = BHARAT_CPU_PARTITION_SYSTEM;
                config->cpu_partitions[i].allowed_sched_classes = BHARAT_SCHED_CLASS_SYSTEM | BHARAT_SCHED_CLASS_FAIR;
            }
        } else if (config->execution_mode == BHARAT_EXEC_MODE_REALTIME) {
            config->cpu_partitions[0].role = BHARAT_CPU_PARTITION_SYSTEM;
            config->cpu_partitions[0].allowed_sched_classes = BHARAT_SCHED_CLASS_SYSTEM;

            config->cpu_partitions[1].role = BHARAT_CPU_PARTITION_REALTIME;
            config->cpu_partitions[1].allowed_sched_classes = BHARAT_SCHED_CLASS_FIFO_RT | BHARAT_SCHED_CLASS_DEADLINE_RT;

            config->cpu_partitions[2].role = BHARAT_CPU_PARTITION_SPARE;
            config->cpu_partitions[2].allowed_sched_classes = BHARAT_SCHED_CLASS_NONE;
        } else if (config->execution_mode == BHARAT_EXEC_MODE_MIXED_CRITICAL) {
            config->cpu_partitions[0].role = BHARAT_CPU_PARTITION_SYSTEM;
            config->cpu_partitions[0].allowed_sched_classes = BHARAT_SCHED_CLASS_SYSTEM;

            config->cpu_partitions[1].role = BHARAT_CPU_PARTITION_REALTIME;
            config->cpu_partitions[1].allowed_sched_classes = BHARAT_SCHED_CLASS_FIFO_RT | BHARAT_SCHED_CLASS_DEADLINE_RT;

            config->cpu_partitions[2].role = BHARAT_CPU_PARTITION_BEST_EFFORT;
            config->cpu_partitions[2].allowed_sched_classes = BHARAT_SCHED_CLASS_FAIR;
        }
    } else if (config->active_cpu_count >= 4) {
        // Spatial Mapping (example base rules)
        if (config->execution_mode == BHARAT_EXEC_MODE_MIXED_CRITICAL) {
            config->cpu_partitions[0].role = BHARAT_CPU_PARTITION_SYSTEM;
            config->cpu_partitions[0].allowed_sched_classes = BHARAT_SCHED_CLASS_SYSTEM;

            config->cpu_partitions[1].role = BHARAT_CPU_PARTITION_REALTIME;
            config->cpu_partitions[1].allowed_sched_classes = BHARAT_SCHED_CLASS_FIFO_RT | BHARAT_SCHED_CLASS_DEADLINE_RT;

            config->cpu_partitions[2].role = BHARAT_CPU_PARTITION_REALTIME;
            config->cpu_partitions[2].allowed_sched_classes = BHARAT_SCHED_CLASS_FIFO_RT | BHARAT_SCHED_CLASS_DEADLINE_RT;

            for (uint32_t i = 3; i < config->active_cpu_count; i++) {
                config->cpu_partitions[i].role = BHARAT_CPU_PARTITION_BEST_EFFORT;
                config->cpu_partitions[i].allowed_sched_classes = BHARAT_SCHED_CLASS_FAIR;
            }
        } else if (config->execution_mode == BHARAT_EXEC_MODE_GENERAL_PURPOSE) {
            for (uint32_t i = 0; i < config->active_cpu_count; i++) {
                config->cpu_partitions[i].role = BHARAT_CPU_PARTITION_SYSTEM;
                config->cpu_partitions[i].allowed_sched_classes = BHARAT_SCHED_CLASS_SYSTEM | BHARAT_SCHED_CLASS_FAIR;
            }
        } else if (config->execution_mode == BHARAT_EXEC_MODE_REALTIME) {
            config->cpu_partitions[0].role = BHARAT_CPU_PARTITION_SYSTEM;
            config->cpu_partitions[0].allowed_sched_classes = BHARAT_SCHED_CLASS_SYSTEM;

            for (uint32_t i = 1; i < config->active_cpu_count; i++) {
                config->cpu_partitions[i].role = BHARAT_CPU_PARTITION_REALTIME;
                config->cpu_partitions[i].allowed_sched_classes = BHARAT_SCHED_CLASS_FIFO_RT | BHARAT_SCHED_CLASS_DEADLINE_RT;
            }
        } else {
            // Default fallback for unknown mode with >= 4 CPUs
            for (uint32_t i = 0; i < config->active_cpu_count; i++) {
                config->cpu_partitions[i].role = BHARAT_CPU_PARTITION_SYSTEM;
                config->cpu_partitions[i].allowed_sched_classes = BHARAT_SCHED_CLASS_SYSTEM | BHARAT_SCHED_CLASS_FAIR;
            }
        }
    }

    // Call invariant validator
    return cpu_partition_validate(config);
}

const bharat_cpu_partition_desc_t* cpu_partition_get(uint32_t cpu_id) {
    const bharat_execution_config_t *cfg = bharat_execution_mode_get_config();
    if (!cfg || cpu_id >= cfg->active_cpu_count) {
        return NULL;
    }
    return &cfg->cpu_partitions[cpu_id];
}

bool cpu_partition_allows_class(uint32_t cpu_id, bharat_sched_class_mask_t class_mask) {
    return cpu_partition_allows_any_class(cpu_id, class_mask);
}

bool cpu_partition_allows_any_class(uint32_t cpu_id, bharat_sched_class_mask_t class_mask) {
    const bharat_cpu_partition_desc_t *desc = cpu_partition_get(cpu_id);
    if (!desc) {
        return false;
    }
    return (desc->allowed_sched_classes & class_mask) != 0;
}

bool cpu_partition_allows_all_classes(uint32_t cpu_id, bharat_sched_class_mask_t class_mask) {
    const bharat_cpu_partition_desc_t *desc = cpu_partition_get(cpu_id);
    if (!desc) {
        return false;
    }
    return (desc->allowed_sched_classes & class_mask) == class_mask;
}

bool cpu_partition_is_housekeeping(uint32_t cpu_id) {
    const bharat_cpu_partition_desc_t *desc = cpu_partition_get(cpu_id);
    return desc ? desc->housekeeping : false;
}

bool cpu_partition_is_rt_dedicated(uint32_t cpu_id) {
    const bharat_cpu_partition_desc_t *desc = cpu_partition_get(cpu_id);
    if (!desc) return false;
    return (desc->role == BHARAT_CPU_PARTITION_REALTIME);
}

bool cpu_partition_can_accept_migration(uint32_t cpu_id, bharat_sched_class_mask_t class_mask) {
    const bharat_cpu_partition_desc_t *desc = cpu_partition_get(cpu_id);
    if (!desc || !desc->allow_migration_in) return false;
    return (desc->allowed_sched_classes & class_mask) != 0;
}

bool cpu_partition_can_send_migration(uint32_t cpu_id, bharat_sched_class_mask_t class_mask) {
    const bharat_cpu_partition_desc_t *desc = cpu_partition_get(cpu_id);
    if (!desc || !desc->allow_migration_out) return false;
    return (desc->allowed_sched_classes & class_mask) != 0;
}

bool cpu_partition_should_receive_irq(uint32_t cpu_id) {
    const bharat_cpu_partition_desc_t *desc = cpu_partition_get(cpu_id);
    return desc ? desc->irq_preferred : false;
}
