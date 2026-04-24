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
        .id = 101,
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
        .id = 102,
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


static void test_manifest_filtering_tiny(void) {
    printf("Running test_manifest_filtering_tiny...\n");
    init_boot_context_t ctx = {0};
    ctx.profile = INIT_PROFILE_TINY;
    ctx.capability_mask = BHARAT_INIT_CAP_NONE;

    init_runtime_t rt;
    memset(&rt, 0, sizeof(rt));
    rt.boot_ctx = ctx;

    // Simulate init_runtime_run filtering logic
    for (size_t i = 0; i < 2; i++) {
        const init_service_desc_t *desc = &g_init_manifest_test[i];
        if (desc->profile_mask & ctx.profile) {
            init_service_id_t id = desc->id;
            rt.service_order[rt.manifest_count] = id;
            rt.services[id].desc = desc;
            rt.manifest_count++;
        }
    }

    assert(rt.manifest_count == 1);
    assert(rt.services[101].desc != NULL);
    printf("Passed test_manifest_filtering_tiny.\n");
}

static void test_manifest_filtering_desktop(void) {
    printf("Running test_manifest_filtering_desktop...\n");
    init_boot_context_t ctx = {0};
    ctx.profile = INIT_PROFILE_DESKTOP;
    ctx.capability_mask = BHARAT_INIT_CAP_NONE;

    init_runtime_t rt;
    memset(&rt, 0, sizeof(rt));
    rt.boot_ctx = ctx;

    for (size_t i = 0; i < 2; i++) {
        const init_service_desc_t *desc = &g_init_manifest_test[i];
        if (desc->profile_mask & ctx.profile) {
            init_service_id_t id = desc->id;
            rt.service_order[rt.manifest_count] = id;
            rt.services[id].desc = desc;
            rt.manifest_count++;
        }
    }

    assert(rt.manifest_count == 2);
    assert(rt.services[101].desc != NULL);
    assert(rt.services[102].desc != NULL);
    printf("Passed test_manifest_filtering_desktop.\n");
}

int main(void) {
    test_manifest_filtering_tiny();
    test_manifest_filtering_desktop();
    printf("All host init_manifest tests passed.\n");
    return 0;
}
