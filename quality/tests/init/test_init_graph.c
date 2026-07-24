#include "../../../core/services/core/init/init_graph.h"
#include "../../../core/services/core/init/init_manifest.h"
#include <bharat/uapi/init/init_boot_context.h>
#include <bharat/uapi/init/init_capability.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

// Mocking g_init_manifest for the test
const init_service_desc_t g_init_manifest[] = {
    { .id = 1, .name = "svc1" },
    { .id = 2, .name = "svc2" }
};
const size_t g_init_manifest_count = 2;

void test_valid_graph() {
    init_boot_context_t ctx = { .capability_mask = BHARAT_INIT_CAP_NONE };
    init_service_id_t deps[] = {1};
    init_service_desc_t manifest[] = {
        { .id = 1, .name = "core_svc", .boot_class = BOOT_CLASS_CORE },
        { .id = 2, .name = "infra_svc", .boot_class = BOOT_CLASS_INFRA, .deps = deps, .dep_count = 1 }
    };

    init_graph_result_t res = init_graph_validate(manifest, 2, &ctx);
    assert(res == INIT_GRAPH_OK);
    printf("Valid graph test passed\n");
}

void test_no_core_service() {
    init_boot_context_t ctx = {0};
    init_service_desc_t manifest[] = {
        { .id = 1, .name = "infra_svc", .boot_class = BOOT_CLASS_INFRA }
    };

    init_graph_result_t res = init_graph_validate(manifest, 1, &ctx);
    assert(res == INIT_GRAPH_ERR_NO_CORE_SERVICE);
    printf("No core service test passed\n");
}

void test_unknown_dep() {
    init_boot_context_t ctx = {0};
    init_service_id_t deps[] = {99};
    init_service_desc_t manifest[] = {
        { .id = 1, .name = "core_svc", .boot_class = BOOT_CLASS_CORE, .deps = deps, .dep_count = 1 }
    };

    init_graph_result_t res = init_graph_validate(manifest, 1, &ctx);
    assert(res == INIT_GRAPH_ERR_UNKNOWN_DEP);
    printf("Unknown dependency test passed\n");
}

void test_cycle() {
    init_boot_context_t ctx = {0};
    init_service_id_t deps1[] = {2};
    init_service_id_t deps2[] = {1};
    init_service_desc_t manifest[] = {
        { .id = 1, .name = "core_svc", .boot_class = BOOT_CLASS_CORE, .deps = deps1, .dep_count = 1 },
        { .id = 2, .name = "other_svc", .boot_class = BOOT_CLASS_CORE, .deps = deps2, .dep_count = 1 }
    };

    init_graph_result_t res = init_graph_validate(manifest, 2, &ctx);
    assert(res == INIT_GRAPH_ERR_CYCLE);
    printf("Cycle detection test passed\n");
}

void test_missing_capability() {
    init_boot_context_t ctx = { .capability_mask = BHARAT_INIT_CAP_NONE };
    init_service_desc_t manifest[] = {
        { .id = 1, .name = "core_svc", .boot_class = BOOT_CLASS_CORE, .required_caps = BHARAT_INIT_CAP_STORAGE }
    };

    init_graph_result_t res = init_graph_validate(manifest, 1, &ctx);
    assert(res == INIT_GRAPH_ERR_REQUIRED_CAP_MISSING);
    printf("Missing capability test passed\n");
}

int main() {
    test_valid_graph();
    test_no_core_service();
    test_unknown_dep();
    test_cycle();
    test_missing_capability();
    printf("All graph validator tests passed!\n");
    return 0;
}
