#ifndef BHARAT_POWER_THERMAL_PERF_H
#define BHARAT_POWER_THERMAL_PERF_H

#include <stdint.h>

#include "hal/power.h"

#define PTP_MAX_WAKE_SOURCES 32U
#define PTP_MAX_POWER_DEVICES 64U
#define PTP_MAX_THERMAL_ZONES 16U
#define PTP_MAX_COOLING_DEVICES 16U
#define PTP_MAX_ACCELERATORS 16U
#define PTP_MAX_CACHE_LEVELS 4U

#ifndef BHARAT_MAX_CORES
#define BHARAT_MAX_CORES 32U
#endif

typedef enum {
    PTP_ARCH_UNKNOWN = 0,
    PTP_ARCH_X86_64,
    PTP_ARCH_ARM64,
    PTP_ARCH_RISCV64
} ptp_arch_t;

typedef enum {
    PTP_PERF_CLASS_UNKNOWN = 0,
    PTP_PERF_CLASS_LITTLE,
    PTP_PERF_CLASS_BALANCED,
    PTP_PERF_CLASS_BIG,
    PTP_PERF_CLASS_ACCELERATOR_HOST
} ptp_perf_class_t;

typedef struct {
    uint32_t package_id;
    uint32_t cluster_id;
    uint32_t core_id;
    uint32_t smt_thread_id;
    uint32_t numa_node_id;
    ptp_perf_class_t perf_class;
    uint32_t max_freq_khz;
} ptp_cpu_topology_t;

typedef struct {
    uint32_t level;
    uint32_t size_kb;
    uint32_t line_size;
    uint32_t shared_cpu_count;
} ptp_cache_level_t;

typedef struct {
    uint32_t id;
    const char *name;
    uint32_t connected_numa_node;
    uint32_t proximity_core;
} ptp_accelerator_t;

typedef struct {
    uint32_t wake_source_id;
    uint64_t wake_count;
    uint64_t last_tick;
} ptp_wake_source_t;

typedef struct {
    uint32_t id;
    int32_t passive_trip_mc;
    int32_t hot_trip_mc;
    int32_t critical_trip_mc;
    int32_t current_temp_mc;
} ptp_thermal_zone_t;

typedef int (*ptp_cooling_apply_t)(uint32_t level, void *ctx);

typedef struct {
    uint32_t id;
    uint32_t max_state;
    uint32_t active_state;
    ptp_cooling_apply_t apply;
    void *ctx;
} ptp_cooling_device_t;

typedef struct {
    ptp_arch_t arch;
    uint32_t discovered_cpu_count;
    uint32_t discovered_numa_nodes;
    uint32_t heterogeneous_cores;
    ptp_cpu_topology_t cpu[BHARAT_MAX_CORES];
    ptp_cache_level_t cache[PTP_MAX_CACHE_LEVELS];
    uint32_t cache_levels;
    ptp_accelerator_t accelerators[PTP_MAX_ACCELERATORS];
    uint32_t accelerator_count;
} ptp_topology_info_t;

int ptp_init(void);

int pm_suspend_system(uint32_t sleep_state);
int pm_resume_system(void);
int pm_set_idle_state(uint32_t core_id, uint32_t idle_state);
int pm_cpufreq_set_hint(uint32_t core_id, uint32_t freq_khz);
int pm_set_device_state(uint32_t device_id, power_device_class_t class_id, int powered_on);
int pm_record_wake_source(uint32_t wake_source_id, uint64_t tick);
const ptp_wake_source_t *pm_get_wake_sources(uint32_t *count);

int thermal_register_zone(const ptp_thermal_zone_t *zone_cfg);
int thermal_register_cooling_device(const ptp_cooling_device_t *cooling_cfg);
int thermal_update_temperature(uint32_t zone_id, int32_t temperature_mc);

const ptp_topology_info_t *perf_topology_get(void);
int perf_topology_register_accelerator(const ptp_accelerator_t *accel);

#endif // BHARAT_POWER_THERMAL_PERF_H
