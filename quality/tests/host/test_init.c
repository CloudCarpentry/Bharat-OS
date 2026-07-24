#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

void bharat_runtime_log(const char *msg) {
    printf("[LOG] %s\n", msg);
}
void bharat_sched_yield(void) {}
void bharat_runtime_shutdown(void) {}

#include "../../core/services/core/init/init_manifest.h"
#include "../../core/services/core/init/init_profile.h"
#include "../../core/services/core/init/init_status.h"
#include "../../core/services/core/init/init_runtime.h"
#include "../../core/services/core/init/init_handoff.h"

void test_init_manifest_filtering(void) {
    printf("Running %s...\n", __func__);
    init_boot_context_t ctx;
    __builtin_memset(&ctx, 0, sizeof(ctx));
    ctx.profile = INIT_PROFILE_SMALL;
    ctx.capability_mask = BHARAT_INIT_CAP_NONE;
    ctx.board_id = BHARAT_INIT_BOARD_ANY;
    ctx.personality_id = BHARAT_PERSONALITY_NATIVE;

    int result = init_runtime_run(&ctx);
    assert(result == 0);
    assert(ctx.safe_mode_requested == false);
}

void test_init_required_failure(void) {
    printf("Running %s...\n", __func__);
    init_boot_context_t ctx;
    __builtin_memset(&ctx, 0, sizeof(ctx));
    ctx.profile = INIT_PROFILE_EMBEDDED_RICH;
    ctx.capability_mask = BHARAT_INIT_CAP_NONE;
    ctx.board_id = BHARAT_INIT_BOARD_ANY;
    ctx.personality_id = BHARAT_PERSONALITY_NATIVE;

    // We expect failure because KERNEL_HEALTH is OK by default,
    // but some services might fail if they are REQUIRED and their start logic fails.
    // In our g_init_manifest, all use stub_start which returns 0.
    // To test failure, we can simulate UNSAFE kernel health.
    ctx.kernel_health.level = INIT_KERNEL_HEALTH_UNSAFE;

    int result = init_runtime_run(&ctx);
    assert(result != 0);
    assert(ctx.safe_mode_requested == true);
}

void test_init_tiny_mode(void) {
    printf("Running %s...\n", __func__);
    init_boot_context_t ctx;
    __builtin_memset(&ctx, 0, sizeof(ctx));
    ctx.profile = INIT_PROFILE_TINY;
    ctx.capability_mask = BHARAT_INIT_CAP_NONE;
    ctx.board_id = BHARAT_INIT_BOARD_ANY;
    ctx.personality_id = BHARAT_PERSONALITY_NATIVE;

    int result = init_runtime_run(&ctx);
    assert(result == 0);
}

int main(void) {
    test_init_manifest_filtering();
    test_init_required_failure();
    test_init_tiny_mode();
    printf("All init tests passed.\n");
    return 0;
}
