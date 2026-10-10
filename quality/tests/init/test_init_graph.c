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
    init_service_id_t deps[] = {63};
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

void test_duplicate_id() {
    init_boot_context_t ctx = { .capability_mask = BHARAT_INIT_CAP_NONE };
    init_service_desc_t manifest[] = {
        { .id = 1, .name = "core_svc1", .boot_class = BOOT_CLASS_CORE },
        { .id = 1, .name = "core_svc2", .boot_class = BOOT_CLASS_CORE }
    };

    init_graph_result_t res = init_graph_validate(manifest, 2, &ctx);
    assert(res == INIT_GRAPH_ERR_DUPLICATE_SERVICE);
    printf("Duplicate ID test passed\n");
}

void test_invalid_id() {
    init_boot_context_t ctx = { .capability_mask = BHARAT_INIT_CAP_NONE };
    init_service_desc_t manifest[] = {
        { .id = INIT_SERVICE_ID_MAX + 1, .name = "invalid_svc", .boot_class = BOOT_CLASS_CORE }
    };

    init_graph_result_t res = init_graph_validate(manifest, 1, &ctx);
    assert(res == INIT_GRAPH_ERR_MALFORMED);
    printf("Invalid ID test passed\n");
}

void test_null_deps() {
    init_boot_context_t ctx = { .capability_mask = BHARAT_INIT_CAP_NONE };
    init_service_desc_t manifest[] = {
        { .id = 1, .name = "core_svc", .boot_class = BOOT_CLASS_CORE, .deps = NULL, .dep_count = 1 }
    };

    init_graph_result_t res = init_graph_validate(manifest, 1, &ctx);
    assert(res == INIT_GRAPH_ERR_MALFORMED);
    printf("Null dependencies test passed\n");
}

void test_malformed_input() {
    init_boot_context_t ctx = { .capability_mask = BHARAT_INIT_CAP_NONE };
    init_service_desc_t manifest[] = {
        { .id = 1, .name = "core_svc", .boot_class = BOOT_CLASS_CORE }
    };

    init_graph_result_t res = init_graph_validate(NULL, 1, &ctx);
    assert(res == INIT_GRAPH_ERR_MALFORMED);

    res = init_graph_validate(manifest, 1, NULL);
    assert(res == INIT_GRAPH_ERR_MALFORMED);

    res = init_graph_validate(manifest, INIT_SERVICE_ID_MAX + 1, &ctx);
    assert(res == INIT_GRAPH_ERR_MALFORMED);

    printf("Malformed input test passed\n");
}

void test_missing_dependency() {
    init_boot_context_t ctx = { .capability_mask = BHARAT_INIT_CAP_NONE };
    init_service_id_t deps[] = {99}; // Not in manifest, but not in g_init_manifest either
    init_service_desc_t manifest[] = {
        { .id = 1, .name = "core_svc", .boot_class = BOOT_CLASS_CORE, .deps = deps, .dep_count = 1 }
    };

    init_graph_result_t res = init_graph_validate(manifest, 1, &ctx);
    assert(res == INIT_GRAPH_ERR_MALFORMED); // 99 is invalid if INIT_SERVICE_ID_MAX is 64
    printf("Missing dependency test passed\n");
}

int main() {
    test_valid_graph();
    test_no_core_service();
    test_unknown_dep();
    test_cycle();
    test_missing_capability();
    test_duplicate_id();
    test_invalid_id();
    test_null_deps();
    test_malformed_input();
    test_missing_dependency();
    printf("All graph validator tests passed!\n");
    return 0;
}
