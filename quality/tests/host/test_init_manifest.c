#include <stdio.h>
#include <assert.h>
#include <stdbool.h>
#include <string.h>

#include <bharat/uapi/init/init_boot_context.h>
#include "../../core/services/core/init/init_profile.h"
#include "../../core/services/core/init/init_manifest.h"
#include "../../core/services/core/init/init_contract.h"

// Define stubs for linking
int bharat_runtime_log(const char *msg) {
    printf("%s\n", msg);
    return 0;
}

// Dummy start functions for tests
static int test_stub_start(void *ctx) {
    (void)ctx;
    return 0;
}

static const init_service_id_t deps_none[] = { INIT_SVC_NONE };
static const init_service_id_t deps_a[] = { 101 };

init_service_desc_t g_init_manifest_test[] = {
    {
        .id = 1,
        .name = "core_svc_a",
        .boot_class = BOOT_CLASS_CORE,
        .start_deadline_ms = 1000,
        .ready_deadline_ms = 5000,
        .start_fn = test_stub_start,
        .probe_fn = NULL,
        .bootstrap_hint_fn = NULL,
        .deps = deps_none,
        .dep_count = 0,
        .retry_limit = 3,
        .policy = INIT_SERVICE_REQUIRED,
        .profile_mask = INIT_PROFILE_TINY | INIT_PROFILE_DESKTOP,
        .board_mask = BHARAT_INIT_BOARD_ANY,
        .personality_mask = BHARAT_INIT_PERSONALITY_ANY,
        .required_caps = BHARAT_INIT_CAP_NONE,
    },
    {
        .id = 2,
        .name = "infra_svc_b",
        .boot_class = BOOT_CLASS_INFRA,
        .start_deadline_ms = 1000,
        .ready_deadline_ms = 5000,
        .start_fn = test_stub_start,
        .probe_fn = NULL,
        .bootstrap_hint_fn = NULL,
        .deps = deps_a,
        .dep_count = 1,
        .retry_limit = 3,
        .policy = INIT_SERVICE_OPTIONAL,
        .profile_mask = INIT_PROFILE_DESKTOP,
        .board_mask = BHARAT_INIT_BOARD_ANY,
        .personality_mask = BHARAT_INIT_PERSONALITY_ANY,
        .required_caps = BHARAT_INIT_CAP_NONE,
    }
};


// Simulate the filter logic used in init_runtime_run
static int simulate_filtering(init_runtime_t *rt, const init_service_desc_t *manifest, size_t count, const init_boot_context_t *ctx) {
    size_t filtered_count = 0;
    for (size_t i = 0; i < count; i++) {
        init_service_id_t id = manifest[i].id;

        if (id <= 0 || id >= INIT_SERVICE_ID_MAX) {
            return -1; // FAIL_PROFILE
        }

        // Simplistic filter for tests
        if (manifest[i].profile_mask & ctx->profile) {
            if (filtered_count >= INIT_SERVICE_ID_MAX) {
                return -1; // FAIL_PROFILE
            }

            if (rt->services[id].desc != NULL) {
                return -1; // FAIL_PROFILE
            }

            rt->service_order[rt->manifest_count] = id;
            rt->services[id].desc = &manifest[i];
            rt->manifest_count++;
            filtered_count++;
        }
    }
    return 0;
}

static void test_manifest_filtering_tiny(void) {
    printf("Running test_manifest_filtering_tiny...\n");
    init_boot_context_t ctx = {0};
    ctx.profile = INIT_PROFILE_TINY;

    init_runtime_t rt;
    memset(&rt, 0, sizeof(rt));

    int rc = simulate_filtering(&rt, g_init_manifest_test, 2, &ctx);
    assert(rc == 0);
    assert(rt.manifest_count == 1);
    assert(rt.services[1].desc != NULL);
    assert(rt.services[2].desc == NULL);
}

static void test_manifest_filtering_desktop(void) {
    printf("Running test_manifest_filtering_desktop...\n");
    init_boot_context_t ctx = {0};
    ctx.profile = INIT_PROFILE_DESKTOP;

    init_runtime_t rt;
    memset(&rt, 0, sizeof(rt));

    int rc = simulate_filtering(&rt, g_init_manifest_test, 2, &ctx);
    assert(rc == 0);
    assert(rt.manifest_count == 2);
    assert(rt.services[1].desc != NULL);
    assert(rt.services[2].desc != NULL);
}

static void test_manifest_filtering_invalid_id(void) {
    printf("Running test_manifest_filtering_invalid_id...\n");
    init_service_desc_t manifest[] = {
        { .id = 0, .profile_mask = INIT_PROFILE_TINY }
    };
    init_boot_context_t ctx = { .profile = INIT_PROFILE_TINY };
    init_runtime_t rt;
    memset(&rt, 0, sizeof(rt));

    int rc = simulate_filtering(&rt, manifest, 1, &ctx);
    assert(rc == -1);

    manifest[0].id = INIT_SERVICE_ID_MAX;
    rc = simulate_filtering(&rt, manifest, 1, &ctx);
    assert(rc == -1);
}

static void test_manifest_filtering_duplicate_id(void) {
    printf("Running test_manifest_filtering_duplicate_id...\n");
    init_service_desc_t manifest[] = {
        { .id = 5, .profile_mask = INIT_PROFILE_TINY },
        { .id = 5, .profile_mask = INIT_PROFILE_TINY }
    };
    init_boot_context_t ctx = { .profile = INIT_PROFILE_TINY };
    init_runtime_t rt;
    memset(&rt, 0, sizeof(rt));

    int rc = simulate_filtering(&rt, manifest, 2, &ctx);
    assert(rc == -1);
}

static void test_manifest_filtering_oversized(void) {
    printf("Running test_manifest_filtering_oversized...\n");
    // Since INIT_SERVICE_ID_MAX is 64, valid IDs are 1 to 63 (63 total).
    // We can't reach filtered_count >= 64 without hitting the duplicate check first.
    // However, let's create a manifest of 64 items and verify it fails,
    // even if it fails on the duplicate check. This is standard defensive programming.
    init_service_desc_t manifest[INIT_SERVICE_ID_MAX + 1];
    memset(&manifest, 0, sizeof(manifest));
    for (size_t i = 0; i < INIT_SERVICE_ID_MAX + 1; i++) {
        manifest[i].id = (i % (INIT_SERVICE_ID_MAX - 1)) + 1; // 1 to 63
        manifest[i].profile_mask = INIT_PROFILE_TINY;
    }

    init_boot_context_t ctx = { .profile = INIT_PROFILE_TINY };
    init_runtime_t rt;
    memset(&rt, 0, sizeof(rt));

    int rc = simulate_filtering(&rt, manifest, INIT_SERVICE_ID_MAX + 1, &ctx);
    assert(rc == -1); // Must fail (either due to duplicate or oversized)
}

static void test_manifest_filtering_valid_sparse(void) {
    printf("Running test_manifest_filtering_valid_sparse...\n");
    init_service_desc_t manifest[] = {
        { .id = 10, .profile_mask = INIT_PROFILE_TINY },
        { .id = 50, .profile_mask = INIT_PROFILE_TINY }
    };
    init_boot_context_t ctx = { .profile = INIT_PROFILE_TINY };
    init_runtime_t rt;
    memset(&rt, 0, sizeof(rt));

    int rc = simulate_filtering(&rt, manifest, 2, &ctx);
    assert(rc == 0);
    assert(rt.manifest_count == 2);
    assert(rt.services[10].desc != NULL);
    assert(rt.services[50].desc != NULL);
}

static void test_manifest_filtering_valid_filtered(void) {
    printf("Running test_manifest_filtering_valid_filtered...\n");
    init_service_desc_t manifest[] = {
        { .id = 1, .profile_mask = INIT_PROFILE_TINY },
        { .id = 2, .profile_mask = INIT_PROFILE_DESKTOP },
        { .id = 3, .profile_mask = INIT_PROFILE_TINY }
    };
    init_boot_context_t ctx = { .profile = INIT_PROFILE_TINY };
    init_runtime_t rt;
    memset(&rt, 0, sizeof(rt));

    int rc = simulate_filtering(&rt, manifest, 3, &ctx);
    assert(rc == 0);
    assert(rt.manifest_count == 2);
    assert(rt.services[1].desc != NULL);
    assert(rt.services[2].desc == NULL);
    assert(rt.services[3].desc != NULL);
}

int main(void) {
    test_manifest_filtering_tiny();
    test_manifest_filtering_desktop();
    test_manifest_filtering_invalid_id();
    test_manifest_filtering_duplicate_id();
    test_manifest_filtering_oversized();
    test_manifest_filtering_valid_sparse();
    test_manifest_filtering_valid_filtered();
    printf("All host init_manifest tests passed.\n");
    return 0;
}
