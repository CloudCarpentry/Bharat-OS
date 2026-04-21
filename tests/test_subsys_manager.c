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
}

int main(void) {
    printf("Running Subsystem Manager tests...\n");

    test_subsys_destroy_null_returns_error();
    test_subsys_destroy_stops_running_instance();
    test_subsys_destroy_on_stopped_instance_is_safe();
    test_subsys_resolve_effective_cpu_mask();

    printf("Subsystem Manager tests passed successfully.\n");
    return 0;
}
