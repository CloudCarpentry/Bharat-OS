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

void subsys_run_boot_tests(const char* name) {
    (void)name;
}



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

static void test_subsys_resolve_effective_cpu_mask(void) {
    hal_cpu_topology_info_t topo_1cpu = {
        .discovered_cpu_count = 1,
        .valid_cpu_mask = 0x1,
        .performance_cluster_mask = 0x1,
        .efficiency_cluster_mask = 0x0,
        .smp_available = false,
        .homogeneous_cores = true
    };

    hal_cpu_topology_info_t topo_4cpu = {
        .discovered_cpu_count = 4,
        .valid_cpu_mask = 0xF,
        .performance_cluster_mask = 0xF,
        .efficiency_cluster_mask = 0x0,
        .smp_available = true,
        .homogeneous_cores = true
    };

    hal_cpu_topology_info_t topo_8cpu_split = {
        .discovered_cpu_count = 8,
        .valid_cpu_mask = 0xFF,
        .performance_cluster_mask = 0xF0, // Top 4 are perf
        .efficiency_cluster_mask = 0x0F,  // Bottom 4 are eff
        .smp_available = true,
        .homogeneous_cores = false
    };

    subsys_alloc_config_t cfg_default = { .policy = SUBSYS_ALLOC_DEFAULT, .preferred_cpu_mask = 0, .cluster_pref = SUBSYS_CLUSTER_PREF_NONE };
    subsys_alloc_config_t cfg_perf_pref = { .policy = SUBSYS_ALLOC_DEFAULT, .preferred_cpu_mask = 0, .cluster_pref = SUBSYS_CLUSTER_PREF_PERF };
    subsys_alloc_config_t cfg_eff_pref = { .policy = SUBSYS_ALLOC_DEFAULT, .preferred_cpu_mask = 0, .cluster_pref = SUBSYS_CLUSTER_PREF_EFF };
    subsys_alloc_config_t cfg_packed = { .policy = SUBSYS_ALLOC_PACKED, .preferred_cpu_mask = 0, .cluster_pref = SUBSYS_CLUSTER_PREF_NONE };
    subsys_alloc_config_t cfg_spread = { .policy = SUBSYS_ALLOC_SPREAD, .preferred_cpu_mask = 0, .cluster_pref = SUBSYS_CLUSTER_PREF_NONE };

    // Test 1: Zero mask -> default fallback to valid mask
    assert(subsys_resolve_effective_cpu_mask(&topo_1cpu, &cfg_default, 0) == 0x1);
    assert(subsys_resolve_effective_cpu_mask(&topo_4cpu, &cfg_default, 0) == 0xF);

    // Test 2: Invalid bit
    assert(subsys_resolve_effective_cpu_mask(&topo_4cpu, &cfg_default, 0x10) == 0xF); // Mask off invalid, falls back

    // Test 3: Mixed valid + invalid
    assert(subsys_resolve_effective_cpu_mask(&topo_4cpu, &cfg_default, 0x11) == 0x1); // Keeps valid bit 0, drops 4

    // Test 4: Full mask
    assert(subsys_resolve_effective_cpu_mask(&topo_4cpu, &cfg_default, 0xFFFFFFFF) == 0xF);

    // Test 5: Preferred CPU Mask from config
    subsys_alloc_config_t cfg_pref_mask = { .policy = SUBSYS_ALLOC_DEFAULT, .preferred_cpu_mask = 0x2, .cluster_pref = SUBSYS_CLUSTER_PREF_NONE };
    assert(subsys_resolve_effective_cpu_mask(&topo_4cpu, &cfg_pref_mask, 0) == 0x2);

    // Test 6: Perf Preferred (8 CPUs split)
    assert(subsys_resolve_effective_cpu_mask(&topo_8cpu_split, &cfg_perf_pref, 0) == 0xF0);

    // Test 7: Eff Preferred (8 CPUs split)
    assert(subsys_resolve_effective_cpu_mask(&topo_8cpu_split, &cfg_eff_pref, 0) == 0x0F);

    // Test 8: Perf Preferred on Homogeneous
    assert(subsys_resolve_effective_cpu_mask(&topo_4cpu, &cfg_perf_pref, 0) == 0xF);

    // Test 9: Eff Preferred on Homogeneous (no eff metadata)
    assert(subsys_resolve_effective_cpu_mask(&topo_4cpu, &cfg_eff_pref, 0) == 0xF);

    // Test 10: PACKED vs SPREAD
    assert(subsys_resolve_effective_cpu_mask(&topo_4cpu, &cfg_packed, 0xF) == 0x1); // Packs to lowest
    assert(subsys_resolve_effective_cpu_mask(&topo_4cpu, &cfg_spread, 0xF) == 0xF); // Spreads to all eligible

    // Test 11: Edge Masks with split topology
    assert(subsys_resolve_effective_cpu_mask(&topo_8cpu_split, &cfg_packed, 0xFFFFFFFF) == 0x1); // Full mask packed

    subsys_alloc_config_t cfg_perf_packed = { .policy = SUBSYS_ALLOC_PACKED, .preferred_cpu_mask = 0, .cluster_pref = SUBSYS_CLUSTER_PREF_PERF };
    assert(subsys_resolve_effective_cpu_mask(&topo_8cpu_split, &cfg_perf_packed, 0) == 0x10); // Perf preferred -> 0xF0 -> packed -> lowest bit of 0xF0 (0x10)

    printf("[PASS] test_subsys_resolve_effective_cpu_mask\n");
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
    test_subsys_resolve_effective_cpu_mask();
    test_mask_sanitization_and_defaults();
    test_packed_and_spread_determinism();
    test_cluster_preferences_and_fallbacks();

    printf("Subsystem Manager tests passed successfully.\n");
    return 0;
}
