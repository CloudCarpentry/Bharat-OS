#include <assert.h>
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
    while(1);
}

void* arch_memcpy(void* dest, const void* src, size_t n, uint32_t flags) { (void)flags; uint8_t *d = dest; const uint8_t *s = src; while (n--) *d++ = *s++; return dest; }
void* arch_memset(void* s, int c, size_t n, uint32_t flags) { (void)flags; uint8_t *p = s; while (n--) *p++ = c; return s; }
void* arch_memmove(void* dest, const void* src, size_t n, uint32_t flags) { (void)flags; uint8_t *d = dest; const uint8_t *s = src; if (d < s) { while (n--) *d++ = *s++; } else { d += n; s += n; while (n--) *--d = *--s; } return dest; }



static void test_subsys_destroy_null_returns_error(void) {
    assert(-1 == subsys_destroy(NULL));
    printf("[PASS] test_subsys_destroy_null_returns_error\n");
}

static void test_subsys_destroy_stops_running_instance(void) {
    subsys_instance_t instance = {0};
    instance.is_running = 1;

    assert(0 == subsys_destroy(&instance));
    assert(0 == instance.is_running);
    printf("[PASS] test_subsys_destroy_stops_running_instance\n");
}

static void test_subsys_destroy_on_stopped_instance_is_safe(void) {
    subsys_instance_t instance = {0};
    instance.is_running = 0;

    assert(0 == subsys_destroy(&instance));
    assert(0 == instance.is_running);
    printf("[PASS] test_subsys_destroy_on_stopped_instance_is_safe\n");
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

static void test_mask_sanitization_and_defaults(void) {
    subsys_alloc_config_t cfg = {
        .policy = SUBSYS_ALLOC_PACKED,
        .preferred_cpu_mask = 0U,
        .cluster_preference = SUBSYS_CLUSTER_PREFERENCE_NONE,
    };

    hal_cpu_topology_info_t topo1 = topo_homogeneous(1U);
    assert(subsys_resolve_effective_cpu_mask(&topo1, 0U, &cfg) == 0x1U);
    assert(subsys_resolve_effective_cpu_mask(&topo1, 0xFFFFFFFFU, &cfg) == 0x1U);

    hal_cpu_topology_info_t topo4 = topo_homogeneous(4U);
    assert(subsys_resolve_effective_cpu_mask(&topo4, 0U, &cfg) == 0x1U);
    assert(subsys_resolve_effective_cpu_mask(&topo4, 1U << 9, &cfg) == 0x1U);
    assert(subsys_resolve_effective_cpu_mask(&topo4, 0xFFFFFFFFU, &cfg) == 0xFU);
    assert((subsys_resolve_effective_cpu_mask(&topo4, 0xAAAAAAAAU, &cfg) & ~topo4.valid_cpu_mask) == 0U);
    printf("[PASS] test_mask_sanitization_and_defaults\n");
}

static void test_packed_and_spread_determinism(void) {
    hal_cpu_topology_info_t topo8 = topo_homogeneous(8U);
    uint32_t requested = 0xFFU;

    subsys_alloc_config_t packed_cfg = {
        .policy = SUBSYS_ALLOC_PACKED,
        .preferred_cpu_mask = requested,
        .cluster_preference = SUBSYS_CLUSTER_PREFERENCE_NONE,
    };
    subsys_alloc_config_t spread_cfg = packed_cfg;
    spread_cfg.policy = SUBSYS_ALLOC_SPREAD;

    uint32_t packed = subsys_resolve_effective_cpu_mask(&topo8, requested, &packed_cfg);
    uint32_t spread = subsys_resolve_effective_cpu_mask(&topo8, requested, &spread_cfg);
    assert(packed == requested);
    assert(spread == requested);
    assert(subsys_resolve_effective_cpu_mask(&topo8, requested, &spread_cfg) == spread);
    printf("[PASS] test_packed_and_spread_determinism\n");
}

static void test_cluster_preferences_and_fallbacks(void) {
    hal_cpu_topology_info_t topo = topo_homogeneous(8U);
    topo.performance_cluster_mask = 0xF0U;
    topo.efficiency_cluster_mask = 0x0FU;
    topo.homogeneous_cores = false;

    subsys_alloc_config_t perf_cfg = {
        .policy = SUBSYS_ALLOC_PACKED,
        .preferred_cpu_mask = 0U,
        .cluster_preference = SUBSYS_CLUSTER_PREFERENCE_PERFORMANCE,
    };
    subsys_alloc_config_t eff_cfg = perf_cfg;
    eff_cfg.cluster_preference = SUBSYS_CLUSTER_PREFERENCE_EFFICIENCY;

    assert(subsys_resolve_effective_cpu_mask(&topo, 0U, &perf_cfg) == 0x10U);
    assert(subsys_resolve_effective_cpu_mask(&topo, 0U, &eff_cfg) == 0x01U);

    topo.performance_cluster_mask = 0U;
    topo.efficiency_cluster_mask = 0U;
    assert(subsys_resolve_effective_cpu_mask(&topo, 0U, &perf_cfg) == 0x01U);
    assert(subsys_resolve_effective_cpu_mask(&topo, 0U, &eff_cfg) == 0x01U);
    printf("[PASS] test_cluster_preferences_and_fallbacks\n");
}

int main(void) {
    printf("Running Subsystem Manager tests...\n");

    test_subsys_destroy_null_returns_error();
    test_subsys_destroy_stops_running_instance();
    test_subsys_destroy_on_stopped_instance_is_safe();
    test_mask_sanitization_and_defaults();
    test_packed_and_spread_determinism();
    test_cluster_preferences_and_fallbacks();

    printf("Subsystem Manager tests passed successfully.\n");
    return 0;
}
