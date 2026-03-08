#include "power_thermal_perf.h"

#include "sched.h"

static ptp_topology_info_t g_topology;
static ptp_wake_source_t g_wake_sources[PTP_MAX_WAKE_SOURCES];
static uint32_t g_wake_source_count;

static ptp_thermal_zone_t g_thermal_zones[PTP_MAX_THERMAL_ZONES];
static uint32_t g_thermal_zone_count;

static ptp_cooling_device_t g_cooling_devices[PTP_MAX_COOLING_DEVICES];
static uint32_t g_cooling_device_count;

static uint32_t g_system_sleep_state;
static uint32_t g_system_suspended;

static ptp_arch_t ptp_detect_arch(void) {
#if defined(__x86_64__)
    return PTP_ARCH_X86_64;
#elif defined(__aarch64__)
    return PTP_ARCH_ARM64;
#elif defined(__riscv)
    return PTP_ARCH_RISCV64;
#else
    return PTP_ARCH_UNKNOWN;
#endif
}

static void perf_topology_build_defaults(void) {
    uint32_t core;

    g_topology.arch = ptp_detect_arch();
    g_topology.discovered_numa_nodes = 1U;
    g_topology.discovered_cpu_count = BHARAT_MAX_CORES;
    g_topology.heterogeneous_cores = 0U;

    for (core = 0U; core < BHARAT_MAX_CORES; ++core) {
        g_topology.cpu[core].package_id = 0U;
        g_topology.cpu[core].cluster_id = core / 4U;
        g_topology.cpu[core].core_id = core;
        g_topology.cpu[core].smt_thread_id = 0U;
        g_topology.cpu[core].numa_node_id = 0U;
        g_topology.cpu[core].max_freq_khz = 2000000U;
        g_topology.cpu[core].perf_class = PTP_PERF_CLASS_BALANCED;
    }

#if defined(__aarch64__)
    for (core = 0U; core < BHARAT_MAX_CORES; ++core) {
        if ((core & 1U) == 0U) {
            g_topology.cpu[core].perf_class = PTP_PERF_CLASS_LITTLE;
            g_topology.cpu[core].max_freq_khz = 1500000U;
            g_topology.heterogeneous_cores = 1U;
        } else {
            g_topology.cpu[core].perf_class = PTP_PERF_CLASS_BIG;
            g_topology.cpu[core].max_freq_khz = 2500000U;
        }
    }
#endif

    g_topology.cache_levels = 3U;
    g_topology.cache[0].level = 1U;
    g_topology.cache[0].size_kb = 64U;
    g_topology.cache[0].line_size = 64U;
    g_topology.cache[0].shared_cpu_count = 1U;

    g_topology.cache[1].level = 2U;
    g_topology.cache[1].size_kb = 512U;
    g_topology.cache[1].line_size = 64U;
    g_topology.cache[1].shared_cpu_count = 4U;

    g_topology.cache[2].level = 3U;
    g_topology.cache[2].size_kb = 4096U;
    g_topology.cache[2].line_size = 64U;
    g_topology.cache[2].shared_cpu_count = BHARAT_MAX_CORES;
}

int ptp_init(void) {
    g_wake_source_count = 0U;
    g_thermal_zone_count = 0U;
    g_cooling_device_count = 0U;
    g_system_sleep_state = 0U;
    g_system_suspended = 0U;

    perf_topology_build_defaults();
    return 0;
}

int pm_suspend_system(uint32_t sleep_state) {
    g_system_sleep_state = sleep_state;
    g_system_suspended = 1U;
    return hal_power_enter_sleep_state(sleep_state);
}

int pm_resume_system(void) {
    g_system_suspended = 0U;
    g_system_sleep_state = 0U;
    return 0;
}

int pm_set_idle_state(uint32_t core_id, uint32_t idle_state) {
    if (core_id >= g_topology.discovered_cpu_count) {
        return -1;
    }

    return hal_power_enter_sleep_state(idle_state);
}

int pm_cpufreq_set_hint(uint32_t core_id, uint32_t freq_khz) {
    pstate_t target_state;

    if (core_id >= g_topology.discovered_cpu_count || freq_khz == 0U) {
        return -1;
    }

    if (freq_khz >= g_topology.cpu[core_id].max_freq_khz) {
        target_state = 0U;
    } else if (freq_khz >= (g_topology.cpu[core_id].max_freq_khz / 2U)) {
        target_state = 1U;
    } else {
        target_state = 2U;
    }

    return hal_power_set_pstate(core_id, target_state);
}

int pm_set_device_state(uint32_t device_id, power_device_class_t class_id, int powered_on) {
    return hal_power_set_device_power_state(device_id, class_id, powered_on, (capability_t *)0);
}

int pm_record_wake_source(uint32_t wake_source_id, uint64_t tick) {
    uint32_t i;

    for (i = 0U; i < g_wake_source_count; ++i) {
        if (g_wake_sources[i].wake_source_id == wake_source_id) {
            g_wake_sources[i].wake_count += 1U;
            g_wake_sources[i].last_tick = tick;
            return 0;
        }
    }

    if (g_wake_source_count >= PTP_MAX_WAKE_SOURCES) {
        return -1;
    }

    g_wake_sources[g_wake_source_count].wake_source_id = wake_source_id;
    g_wake_sources[g_wake_source_count].wake_count = 1U;
    g_wake_sources[g_wake_source_count].last_tick = tick;
    g_wake_source_count += 1U;
    return 0;
}

const ptp_wake_source_t *pm_get_wake_sources(uint32_t *count) {
    if (count) {
        *count = g_wake_source_count;
    }
    return g_wake_sources;
}

int thermal_register_zone(const ptp_thermal_zone_t *zone_cfg) {
    if (!zone_cfg || g_thermal_zone_count >= PTP_MAX_THERMAL_ZONES) {
        return -1;
    }

    g_thermal_zones[g_thermal_zone_count] = *zone_cfg;
    g_thermal_zone_count += 1U;
    return 0;
}

int thermal_register_cooling_device(const ptp_cooling_device_t *cooling_cfg) {
    if (!cooling_cfg || g_cooling_device_count >= PTP_MAX_COOLING_DEVICES) {
        return -1;
    }

    g_cooling_devices[g_cooling_device_count] = *cooling_cfg;
    g_cooling_device_count += 1U;
    return 0;
}

int thermal_update_temperature(uint32_t zone_id, int32_t temperature_mc) {
    uint32_t i;
    uint32_t max_level = 0U;

    for (i = 0U; i < g_thermal_zone_count; ++i) {
        if (g_thermal_zones[i].id == zone_id) {
            g_thermal_zones[i].current_temp_mc = temperature_mc;

            if (temperature_mc >= g_thermal_zones[i].critical_trip_mc) {
                max_level = 3U;
            } else if (temperature_mc >= g_thermal_zones[i].hot_trip_mc) {
                max_level = 2U;
            } else if (temperature_mc >= g_thermal_zones[i].passive_trip_mc) {
                max_level = 1U;
            }
            break;
        }
    }

    if (i == g_thermal_zone_count) {
        return -1;
    }

    for (i = 0U; i < g_cooling_device_count; ++i) {
        uint32_t requested = max_level;
        if (requested > g_cooling_devices[i].max_state) {
            requested = g_cooling_devices[i].max_state;
        }

        g_cooling_devices[i].active_state = requested;
        if (g_cooling_devices[i].apply) {
            (void)g_cooling_devices[i].apply(requested, g_cooling_devices[i].ctx);
        }
    }

    if (max_level >= 2U) {
        (void)sched_throttle_core(0U);
    }

    return 0;
}

const ptp_topology_info_t *perf_topology_get(void) {
    return &g_topology;
}

int perf_topology_register_accelerator(const ptp_accelerator_t *accel) {
    if (!accel || g_topology.accelerator_count >= PTP_MAX_ACCELERATORS) {
        return -1;
    }

    g_topology.accelerators[g_topology.accelerator_count] = *accel;
    g_topology.accelerator_count += 1U;
    return 0;
}

int hal_power_set_pstate(uint32_t core_id, pstate_t state) {
    (void)core_id;
    (void)state;
    return 0;
}

int hal_power_apply_dvfs_hint(uint32_t core_id, dvfs_prediction_hint_t *hint, capability_t *cap) {
    (void)core_id;
    (void)hint;
    (void)cap;
    return 0;
}

int hal_power_clock_gate(uint32_t device_id, int enable) {
    (void)device_id;
    (void)enable;
    return 0;
}

int hal_power_set_device_power_state(uint32_t device_id, power_device_class_t class_id, int powered_on, capability_t *cap) {
    (void)device_id;
    (void)class_id;
    (void)powered_on;
    (void)cap;
    return 0;
}

int hal_power_enter_sleep_state(uint32_t cstate) {
    (void)cstate;
    return 0;
}
