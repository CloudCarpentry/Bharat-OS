#include <profile/execution_mode.h>
#include <hal/hal_cpu_topology.h>
#include <sched/cpu_partition.h>
#include <console/console_core.h>
#include <stddef.h>

static bharat_execution_config_t g_exec_config = {0};
static bool g_exec_config_initialized = false;

// Mock CMake overrides. In reality, these come from CFLAGS/defines
#ifndef BHARAT_CONFIG_SYSTEM_PROFILE
#define BHARAT_CONFIG_SYSTEM_PROFILE BHARAT_SYSTEM_PROFILE_UNKNOWN
#endif

#ifndef BHARAT_CONFIG_EXECUTION_MODE
#define BHARAT_CONFIG_EXECUTION_MODE BHARAT_EXEC_MODE_UNKNOWN
#endif

#ifndef BHARAT_CONFIG_LOGICAL_CPU_COUNT
#define BHARAT_CONFIG_LOGICAL_CPU_COUNT 0
#endif

int bharat_execution_mode_init(void) {
    if (g_exec_config_initialized) {
        return 0;
    }

    g_exec_config.system_profile = BHARAT_CONFIG_SYSTEM_PROFILE;
    g_exec_config.execution_mode = BHARAT_CONFIG_EXECUTION_MODE;

    hal_cpu_topology_info_t topo;
    if (hal_cpu_topology_query(&topo)) {
        g_exec_config.discovered_cpu_count = topo.discovered_cpu_count;
    } else {
        g_exec_config.discovered_cpu_count = 1;
    }

    uint32_t override_cpu_count = BHARAT_CONFIG_LOGICAL_CPU_COUNT;
    if (override_cpu_count > 0) {
        g_exec_config.active_cpu_count = override_cpu_count;
    } else {
        g_exec_config.active_cpu_count = g_exec_config.discovered_cpu_count;
    }

    // Bounds check
    if (g_exec_config.active_cpu_count > BHARAT_MAX_CPU_PARTITIONS) {
        g_exec_config.active_cpu_count = BHARAT_MAX_CPU_PARTITIONS;
    }

    int rc = cpu_partition_init(&g_exec_config);
    if (rc == 0) {
        g_exec_config_initialized = true;
    }

    return rc;
}

const bharat_execution_config_t* bharat_execution_mode_get_config(void) {
    if (!g_exec_config_initialized) {
        return NULL;
    }
    return &g_exec_config;
}

void bharat_execution_mode_print_summary(void) {
    if (!g_exec_config_initialized) {
        return;
    }

    // A proper ksnprintf/logging framework should be introduced later.
    // For now, use console_write_raw to satisfy the PR requirement without introducing libc/printf.
#ifndef TESTING
    const char *msg_start = "--- Execution Mode Boot Summary ---\n";
    console_write_raw(msg_start, string_length(msg_start));

    // Detailed integer/enum logging will be added once a formatting helper is available.
    // For the scope of this framework scaffolding, a static success log avoids linking issues.
    const char *msg_ok = "Execution Mode Framework initialized successfully.\n";
    console_write_raw(msg_ok, string_length(msg_ok));

    const char *msg_end = "-----------------------------------\n";
    console_write_raw(msg_end, string_length(msg_end));
#endif
}
