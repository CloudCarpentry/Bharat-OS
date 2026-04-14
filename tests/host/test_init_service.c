#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <kernel/status.h>

#include "../../services/core/init/init_manifest.h"
#include "../../services/core/init/init_runtime.h"
#include "../../services/core/init/init_status.h"

void bharat_runtime_log(const char *msg) {
    printf("%s\n", msg);
}

// We use a global counter to tell `dummy_start` whether to succeed or fail.
int g_fail_start_for_service_idx = -1;
int g_current_service_idx = 0;
int g_current_attempts = 0;

int dummy_start(void *ctx) {
    // Determine which service we are by looking at g_current_service_idx
    if (g_current_service_idx == g_fail_start_for_service_idx) {
        g_current_attempts++;
        return K_ERR_INTERNAL_BUG;
    }
    g_current_service_idx++;
    return K_OK;
}

void test_init_manifest_filtering(void) {
    printf("Running test_init_manifest_filtering...\n");
    init_boot_context_t ctx = {
        .profile = BHARAT_INIT_PROFILE_TINY,
        .board_id = 0,
        .cap_mask = ~0ULL,
    };

    int rc = init_runtime_start(&ctx);
    assert(rc == K_OK);

    init_service_runtime_t *runtimes = init_status_get_runtimes();
    // In tiny profile, there is 1 optional service: namesvc
    assert(runtimes[0].state == INIT_SERVICE_RUNNING);
}

void test_init_dependency_order(void) {
    printf("Running test_init_dependency_order...\n");

    // Test that if we use SMALL (where devmgr-lite depends on namesvc), it starts correctly
    init_boot_context_t ctx = {
        .profile = BHARAT_INIT_PROFILE_SMALL,
        .board_id = 0,
        .cap_mask = ~0ULL,
    };

    int rc = init_runtime_start(&ctx);
    assert(rc == K_OK);

    init_service_runtime_t *runtimes = init_status_get_runtimes();
    assert(strcmp(runtimes[0].desc->name, "namesvc") == 0);
    assert(runtimes[0].state == INIT_SERVICE_RUNNING);
    assert(strcmp(runtimes[1].desc->name, "devmgr-lite") == 0);
    assert(runtimes[1].state == INIT_SERVICE_RUNNING);
}

void test_init_optional_failure(void) {
    printf("Running test_init_optional_failure...\n");

    // We want index 1 (boot_displayd in MOBILE) to fail
    g_fail_start_for_service_idx = 1;
    g_current_service_idx = 0;
    g_current_attempts = 0;

    init_boot_context_t ctx = {
        .profile = BHARAT_INIT_PROFILE_MOBILE,
        .board_id = 0,
        .cap_mask = ~0ULL,
    };

    int rc = init_runtime_start(&ctx);
    assert(rc == K_OK); // boot does not fail because it's optional

    init_service_runtime_t *runtimes = init_status_get_runtimes();
    assert(runtimes[0].state == INIT_SERVICE_RUNNING);
    assert(runtimes[1].state == INIT_SERVICE_FAILED);

    g_fail_start_for_service_idx = -1;
}

void test_init_required_failure(void) {
    printf("Running test_init_required_failure...\n");

    // We want index 0 (namesvc in MOBILE, REQUIRED) to fail
    g_fail_start_for_service_idx = 0;
    g_current_service_idx = 0;
    g_current_attempts = 0;

    init_boot_context_t ctx = {
        .profile = BHARAT_INIT_PROFILE_MOBILE,
        .board_id = 0,
        .cap_mask = ~0ULL,
    };

    int rc = init_runtime_start(&ctx);
    assert(rc != K_OK); // boot fails because it's required

    init_service_runtime_t *runtimes = init_status_get_runtimes();
    assert(runtimes[0].state == INIT_SERVICE_FAILED);

    g_fail_start_for_service_idx = -1;
}

void test_init_retry_limit(void) {
    printf("Running test_init_retry_limit...\n");

    // We want index 0 (namesvc in DESKTOP, REQUIRED, limit 3) to fail
    g_fail_start_for_service_idx = 0;
    g_current_service_idx = 0;
    g_current_attempts = 0;

    init_boot_context_t ctx = {
        .profile = BHARAT_INIT_PROFILE_DESKTOP,
        .board_id = 0,
        .cap_mask = ~0ULL,
    };

    int rc = init_runtime_start(&ctx);
    assert(rc != K_OK);

    init_service_runtime_t *runtimes = init_status_get_runtimes();
    assert(runtimes[0].state == INIT_SERVICE_FAILED);
    assert(runtimes[0].attempts == 4); // 1 initial + 3 retries = 4

    g_fail_start_for_service_idx = -1;
}

void test_init_handoff_stub(void) {
    printf("Running test_init_handoff_stub...\n");

    init_boot_context_t ctx = {
        .profile = BHARAT_INIT_PROFILE_TINY,
    };

    int rc = init_handoff_to_supervisor(&ctx);
#if defined(BHARAT_INIT_ENABLE_HANDOFF)
    assert(rc == K_OK);
#else
    assert(rc == K_ERR_UNSUPPORTED);
#endif
}

int main() {
    printf("Starting Init Service tests...\n");

    test_init_manifest_filtering();
    test_init_dependency_order();
    test_init_optional_failure();
    test_init_required_failure();
    test_init_retry_limit();
    test_init_handoff_stub();

    printf("All Init Service tests passed!\n");
    return 0;
}
