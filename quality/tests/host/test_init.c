#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

// Mocks to replace bharat_runtime_log and sched_yield
void bharat_runtime_log(const char *msg) {
    printf("[LOG] %s\n", msg);
}
void bharat_sched_yield(void) {}

// Include headers to test
#include "../../core/services/core/init/init_manifest.h"
#include "../../core/services/core/init/init_profile.h"
#include "../../core/services/core/init/init_status.h"
#include "../../core/services/core/init/init_runtime.h"
#include "../../core/services/core/init/init_handoff.h"

#define BHARAT_INIT_BOARD_ANY_TEST           (0xFFFFFFFFFFFFFFFFULL)
#define BHARAT_INIT_PERSONALITY_ANY_TEST     (0xFFFFFFFFFFFFFFFFULL)

// Define custom test manifest here to decouple from actual generic manifest
static int mock_start_success(void *ctx) { return 0; }
static int mock_start_fail(void *ctx) { return -1; }

static const init_service_id_t deps_none[] = { INIT_SVC_NONE };
static const init_service_id_t deps_A[] = { 101 };

const init_service_desc_t g_init_manifest[] = {
    {
        .id = 101, // Service A
        .name = "service_A",
        .start_fn = mock_start_success,
        .deps = deps_none,
        .dep_count = 0,
        .retry_limit = 1,
        .policy = INIT_SERVICE_REQUIRED,
        .profile_mask = BHARAT_INIT_PROFILE_SMALL | BHARAT_INIT_PROFILE_EMBEDDED_RICH,
        .board_mask = BHARAT_INIT_BOARD_ANY,
        .personality_mask = BHARAT_INIT_PERSONALITY_ANY,
        .required_caps = BHARAT_INIT_CAP_NONE,
    },
    {
        .id = 102, // Service B (Depends on A)
        .name = "service_B",
        .start_fn = mock_start_success,
        .deps = deps_A,
        .dep_count = 1,
        .retry_limit = 1,
        .policy = INIT_SERVICE_OPTIONAL,
        .profile_mask = BHARAT_INIT_PROFILE_SMALL | BHARAT_INIT_PROFILE_EMBEDDED_RICH,
        .board_mask = BHARAT_INIT_BOARD_ANY,
        .personality_mask = BHARAT_INIT_PERSONALITY_ANY,
        .required_caps = BHARAT_INIT_CAP_NONE,
    },
    {
        .id = 103, // Service C (Fails, required)
        .name = "service_C_fail_req",
        .start_fn = mock_start_fail,
        .deps = deps_none,
        .dep_count = 0,
        .retry_limit = 2,
        .policy = INIT_SERVICE_REQUIRED,
        .profile_mask = BHARAT_INIT_PROFILE_EMBEDDED_RICH,
        .board_mask = BHARAT_INIT_BOARD_ANY,
        .personality_mask = BHARAT_INIT_PERSONALITY_ANY,
        .required_caps = BHARAT_INIT_CAP_NONE,
    },
    {
        .id = 104, // Service D (Fails, optional)
        .name = "service_D_fail_opt",
        .start_fn = mock_start_fail,
        .deps = deps_none,
        .dep_count = 0,
        .retry_limit = 1,
        .policy = INIT_SERVICE_OPTIONAL,
        .profile_mask = BHARAT_INIT_PROFILE_SMALL,
        .board_mask = BHARAT_INIT_BOARD_ANY,
        .personality_mask = BHARAT_INIT_PERSONALITY_ANY,
        .required_caps = BHARAT_INIT_CAP_NONE,
    },
    {
        .id = 105, // Service E (Tiny only)
        .name = "service_E_tiny",
        .start_fn = mock_start_success,
        .deps = deps_none,
        .dep_count = 0,
        .retry_limit = 1,
        .policy = INIT_SERVICE_REQUIRED,
        .profile_mask = BHARAT_INIT_PROFILE_TINY,
        .board_mask = BHARAT_INIT_BOARD_ANY,
        .personality_mask = BHARAT_INIT_PERSONALITY_ANY,
        .required_caps = BHARAT_INIT_CAP_NONE,
    },
    {
        .id = 106, // Service F (Requires MMU)
        .name = "service_F_mmu",
        .start_fn = mock_start_success,
        .deps = deps_none,
        .dep_count = 0,
        .retry_limit = 1,
        .policy = INIT_SERVICE_OPTIONAL,
        .profile_mask = BHARAT_INIT_PROFILE_SMALL | BHARAT_INIT_PROFILE_EMBEDDED_RICH,
        .board_mask = BHARAT_INIT_BOARD_ANY,
        .personality_mask = BHARAT_INIT_PERSONALITY_ANY,
        .required_caps = BHARAT_INIT_CAP_MMU,
    }
};

const size_t g_init_manifest_count = sizeof(g_init_manifest) / sizeof(g_init_manifest[0]);


// --- Test Cases ---

void test_init_manifest_filtering(void) {
    printf("Running %s...\n", __func__);
    init_boot_context_t ctx;
    ctx.profile = BHARAT_INIT_PROFILE_SMALL;
    ctx.cap_mask = BHARAT_INIT_CAP_NONE;
    ctx.board_id = BHARAT_INIT_BOARD_ANY;
    ctx.personality_id = BHARAT_INIT_PERSONALITY_ANY;
    ctx.safe_mode = false;

    int result = init_runtime_run(&ctx);
    assert(result == 0); // No required services fail for SMALL profile
    assert(ctx.safe_mode == false);
}

void test_init_optional_failure(void) {
    printf("Running %s...\n", __func__);
    init_boot_context_t ctx;
    ctx.profile = BHARAT_INIT_PROFILE_SMALL; // Service D fails, but it is optional
    ctx.cap_mask = BHARAT_INIT_CAP_NONE;
    ctx.board_id = BHARAT_INIT_BOARD_ANY;
    ctx.personality_id = BHARAT_INIT_PERSONALITY_ANY;
    ctx.safe_mode = false;

    int result = init_runtime_run(&ctx);
    assert(result == 0); // Should not fail boot
    assert(ctx.safe_mode == false);
}

void test_init_required_failure(void) {
    printf("Running %s...\n", __func__);
    init_boot_context_t ctx;
    ctx.profile = BHARAT_INIT_PROFILE_EMBEDDED_RICH; // Service C is required and fails
    ctx.cap_mask = BHARAT_INIT_CAP_NONE;
    ctx.board_id = BHARAT_INIT_BOARD_ANY;
    ctx.personality_id = BHARAT_INIT_PERSONALITY_ANY;
    ctx.safe_mode = false;

    int result = init_runtime_run(&ctx);
    assert(result == -EFAULT); // Should fail boot
    assert(ctx.safe_mode == true);
}

void test_init_tiny_mode(void) {
    printf("Running %s...\n", __func__);
    init_boot_context_t ctx;
    ctx.profile = BHARAT_INIT_PROFILE_TINY;
    ctx.cap_mask = BHARAT_INIT_CAP_NONE;
    ctx.board_id = BHARAT_INIT_BOARD_ANY;
    ctx.personality_id = BHARAT_INIT_PERSONALITY_ANY;
    ctx.safe_mode = false;

    int result = init_runtime_run(&ctx);
    assert(result == 0); // Tiny service starts fine
}

void test_init_handoff_stub(void) {
    printf("Running %s...\n", __func__);
    init_boot_context_t ctx;
    ctx.profile = BHARAT_INIT_PROFILE_SMALL;
    ctx.cap_mask = BHARAT_INIT_CAP_NONE;

    int res = init_handoff_to_supervisor(&ctx);
    // Since BHARAT_INIT_ENABLE_HANDOFF isn't defined here, it might return 0 or -ENOSYS
    assert(res == 0 || res == -ENOSYS);
}

int main(void) {
    test_init_manifest_filtering();
    test_init_optional_failure();
    test_init_required_failure();
    test_init_tiny_mode();
    test_init_handoff_stub();
    printf("All init tests passed.\n");
    return 0;
}
