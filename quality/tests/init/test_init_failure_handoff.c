#include "../../../core/services/core/init/init_runtime.h"
#include <assert.h>
#include <errno.h>
#include <stdio.h>

static int launch_error;
static int handoff_result;
static unsigned handoff_calls;

static int launch(void *ctx) {
    (void)ctx;
    return launch_error;
}

const init_service_desc_t g_init_manifest[] = {
    {.id = INIT_SVC_NAMESVC, .name = "namesvc", .boot_class = BOOT_CLASS_CORE,
     .start_fn = launch, .policy = INIT_SERVICE_REQUIRED,
     .profile_mask = BHARAT_INIT_PROFILE_DESKTOP,
     .board_mask = BHARAT_INIT_BOARD_ANY,
     .personality_mask = BHARAT_INIT_PERSONALITY_ANY},
    {.id = INIT_SVC_SERVICEMGR, .name = "servicemgr", .boot_class = BOOT_CLASS_INFRA,
     .start_fn = launch, .policy = INIT_SERVICE_REQUIRED,
     .profile_mask = BHARAT_INIT_PROFILE_DESKTOP,
     .board_mask = BHARAT_INIT_BOARD_ANY,
     .personality_mask = BHARAT_INIT_PERSONALITY_ANY}
};
const size_t g_init_manifest_count = sizeof(g_init_manifest) / sizeof(g_init_manifest[0]);

void bharat_runtime_log(const char *msg) { (void)msg; }
int bharat_runtime_now_ns(uint64_t *out) { *out = 0; return 0; }
int bharat_bootstrap_poll(uint32_t cap, bh_bootstrap_service_event_t *event) {
    (void)cap; (void)event; return -1;
}
int bharat_sched_yield(void) { return 0; }

int init_handoff_to_supervisor(const init_boot_context_t *ctx, struct init_runtime_s *rt) {
    (void)ctx;
    (void)rt;
    ++handoff_calls;
    return handoff_result;
}

int main(void) {
    const int launch_results[] = {-EIO, 0}; /* failure and accepted-but-not-ready */
    const int handoff_results[] = {0, -ENOENT, -EIO};
    for (size_t i = 0; i < sizeof(launch_results) / sizeof(launch_results[0]); ++i) {
        for (size_t j = 0; j < sizeof(handoff_results) / sizeof(handoff_results[0]); ++j) {
            init_boot_context_t ctx = {.profile = INIT_PROFILE_DESKTOP};
            launch_error = launch_results[i];
            handoff_result = handoff_results[j];
            handoff_calls = 0;
            assert(init_runtime_run(&ctx) == -EFAULT);
            assert(ctx.safe_mode_requested);
            assert(handoff_calls == 0);
        }
    }
    printf("PASS: six required-service failure/readiness cases preserve safe mode without handoff\n");
    return 0;
}
