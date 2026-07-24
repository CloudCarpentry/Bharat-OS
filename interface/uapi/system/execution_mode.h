#ifndef BHARAT_UAPI_SYSTEM_EXECUTION_MODE_H
#define BHARAT_UAPI_SYSTEM_EXECUTION_MODE_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    BHARAT_SYSTEM_PROFILE_UNKNOWN = 0,
    BHARAT_SYSTEM_PROFILE_AUTOMOBILE,
    BHARAT_SYSTEM_PROFILE_MOBILE,
    BHARAT_SYSTEM_PROFILE_APPLIANCE,
    BHARAT_SYSTEM_PROFILE_DESKTOP,
    BHARAT_SYSTEM_PROFILE_EDGE,
    BHARAT_SYSTEM_PROFILE_MEDICAL,
    BHARAT_SYSTEM_PROFILE_DRONE
} bharat_system_profile_t;

typedef enum {
    BHARAT_EXEC_MODE_UNKNOWN = 0,
    BHARAT_EXEC_MODE_REALTIME,
    BHARAT_EXEC_MODE_GENERAL_PURPOSE,
    BHARAT_EXEC_MODE_MIXED_CRITICAL
} bharat_execution_mode_t;

typedef enum {
    BHARAT_CPU_PARTITION_NONE = 0,
    BHARAT_CPU_PARTITION_SYSTEM,
    BHARAT_CPU_PARTITION_REALTIME,
    BHARAT_CPU_PARTITION_BEST_EFFORT,
    BHARAT_CPU_PARTITION_ISOLATED,
    BHARAT_CPU_PARTITION_SPARE
} bharat_cpu_partition_role_t;

typedef enum {
    BHARAT_SCHED_CLASS_NONE        = 0,
    BHARAT_SCHED_CLASS_SYSTEM      = 1u << 0,
    BHARAT_SCHED_CLASS_FIFO_RT     = 1u << 1,
    BHARAT_SCHED_CLASS_DEADLINE_RT = 1u << 2,
    BHARAT_SCHED_CLASS_FAIR        = 1u << 3,
    BHARAT_SCHED_CLASS_IDLE        = 1u << 4
} bharat_sched_class_mask_t;

typedef enum {
    BHARAT_PARTITION_STRATEGY_NONE = 0,
    BHARAT_PARTITION_STRATEGY_SPATIAL,
    BHARAT_PARTITION_STRATEGY_TEMPORAL
} bharat_partition_strategy_t;

typedef struct {
    uint32_t cpu_id;
    bharat_cpu_partition_role_t role;
    uint32_t allowed_sched_classes;
    bool housekeeping;
    bool allow_migration_in;
    bool allow_migration_out;
    bool irq_preferred;
} bharat_cpu_partition_desc_t;

#define BHARAT_MAX_CPU_PARTITIONS 32

typedef struct {
    bharat_system_profile_t system_profile;
    bharat_execution_mode_t execution_mode;
    uint32_t discovered_cpu_count;
    uint32_t active_cpu_count;
    bharat_partition_strategy_t partition_strategy;
    bool temporal_partition_fallback;
    bool strict_partitioning;
    bool allow_controlled_spillover;
    uint32_t housekeeping_cpu_mask;
    uint32_t realtime_cpu_mask;
    uint32_t best_effort_cpu_mask;
    uint32_t system_cpu_mask;
    uint32_t isolated_cpu_mask;
    bharat_cpu_partition_desc_t cpu_partitions[BHARAT_MAX_CPU_PARTITIONS];
} bharat_execution_config_t;

#ifdef __cplusplus
}
#endif

#endif // BHARAT_UAPI_SYSTEM_EXECUTION_MODE_H
