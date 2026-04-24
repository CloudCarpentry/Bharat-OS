#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "../services/core/subsysmgr/include/subsys.h"

// Stubs for subsys_test_runner.c
#include "../services/core/subsysmgr/include/bharat/subsys_test.h"
const subsys_test_t __subsys_tests_start[0] = {};
const subsys_test_t __subsys_tests_end[0] = {};

void hal_serial_write(char c) {
    (void)c;
}

void kernel_panic(const char* msg) {
    (void)msg;
    while (1) {}
}

void* arch_memcpy(void* dest, const void* src, size_t n, uint32_t flags) {
    (void)flags;
    uint8_t* d = dest;
    const uint8_t* s = src;
    while (n--) *d++ = *s++;
    return dest;
}

void* arch_memset(void* s, int c, size_t n, uint32_t flags) {
    (void)flags;
    uint8_t* p = s;
    while (n--) *p++ = (uint8_t)c;
    return s;
}

void* arch_memmove(void* dest, const void* src, size_t n, uint32_t flags) {
    (void)flags;
    uint8_t* d = dest;
    const uint8_t* s = src;
    if (d < s) {
        while (n--) *d++ = *s++;
    } else {
        d += n;
        s += n;
        while (n--) *--d = *--s;
    }
    return dest;
}

void subsys_run_boot_tests(const char* name) {
    (void)name;
}

static hal_cpu_topology_info_t topo_homogeneous(uint32_t cpu_count) {
    hal_cpu_topology_info_t topo = {0};
    topo.discovered_cpu_count = cpu_count;
    topo.valid_cpu_mask = (cpu_count >= 32U) ? UINT32_MAX : ((cpu_count == 0U) ? 0U : ((1U << cpu_count) - 1U));
    topo.performance_cluster_mask = topo.valid_cpu_mask;
    topo.efficiency_cluster_mask = 0U;
    topo.smp_available = (cpu_count > 1U);
    topo.homogeneous_cores = true;
    return topo;
}

static void test_subsys_destroy_null_returns_error(void) {
    assert(subsys_destroy(NULL) == -1);
    printf("[PASS] test_subsys_destroy_null_returns_error\n");
}

static void test_subsys_destroy_stops_running_instance(void) {
    subsys_instance_t instance = {0};
    instance.is_running = 1;

    assert(subsys_destroy(&instance) == 0);
    assert(instance.is_running == 0);
    printf("[PASS] test_subsys_destroy_stops_running_instance\n");
}

static void test_subsys_resolve_effective_cpu_mask(void) {
    hal_cpu_topology_info_t topo_4cpu = topo_homogeneous(4);
    hal_cpu_topology_info_t topo_8cpu_split = topo_homogeneous(8);
    topo_8cpu_split.performance_cluster_mask = 0xF0U;
    topo_8cpu_split.efficiency_cluster_mask = 0x0FU;
    topo_8cpu_split.homogeneous_cores = false;

    subsys_alloc_config_t cfg_default = {
        .policy = SUBSYS_ALLOC_DEFAULT,
        .preferred_cpu_mask = 0U,
        .cluster_pref = SUBSYS_CLUSTER_PREF_NONE,
    };
    subsys_alloc_config_t cfg_perf = cfg_default;
    cfg_perf.cluster_pref = SUBSYS_CLUSTER_PREF_PERF;

    subsys_alloc_config_t cfg_eff = cfg_default;
    cfg_eff.cluster_pref = SUBSYS_CLUSTER_PREF_EFF;

    subsys_alloc_config_t cfg_packed = cfg_default;
    cfg_packed.policy = SUBSYS_ALLOC_PACKED;

    subsys_alloc_config_t cfg_spread = cfg_default;
    cfg_spread.policy = SUBSYS_ALLOC_SPREAD;

    assert(subsys_resolve_effective_cpu_mask(&topo_4cpu, 0U, &cfg_default) == 0xFU);
    assert(subsys_resolve_effective_cpu_mask(&topo_4cpu, 0x10U, &cfg_default) == 0xFU);
    assert(subsys_resolve_effective_cpu_mask(&topo_4cpu, 0x11U, &cfg_default) == 0x1U);

    cfg_default.preferred_cpu_mask = 0x2U;
    assert(subsys_resolve_effective_cpu_mask(&topo_4cpu, 0U, &cfg_default) == 0x2U);

    assert(subsys_resolve_effective_cpu_mask(&topo_8cpu_split, 0U, &cfg_perf) == 0xF0U);
    assert(subsys_resolve_effective_cpu_mask(&topo_8cpu_split, 0U, &cfg_eff) == 0x0FU);

    assert(subsys_resolve_effective_cpu_mask(&topo_4cpu, 0xFU, &cfg_packed) == 0x1U);
    assert(subsys_resolve_effective_cpu_mask(&topo_4cpu, 0xFU, &cfg_spread) == 0xFU);

    subsys_alloc_config_t cfg_perf_packed = {
        .policy = SUBSYS_ALLOC_PACKED,
        .preferred_cpu_mask = 0U,
        .cluster_pref = SUBSYS_CLUSTER_PREF_PERF,
    };
    assert(subsys_resolve_effective_cpu_mask(&topo_8cpu_split, 0U, &cfg_perf_packed) == 0x10U);

    printf("[PASS] test_subsys_resolve_effective_cpu_mask\n");
}

int main(void) {
    printf("Running Subsystem Manager tests...\n");

    test_subsys_destroy_null_returns_error();
    test_subsys_destroy_stops_running_instance();
    test_subsys_resolve_effective_cpu_mask();

    printf("Subsystem Manager tests passed successfully.\n");
    return 0;
}
