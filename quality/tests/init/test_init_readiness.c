#include "../../../core/services/core/init/init_runtime.h"
#include <bharat/uapi/syscall/bh_syscall_status.h>
#include <assert.h>
#include <errno.h>
#include <stdio.h>

static unsigned events[3];
static unsigned launched[3];
static uint64_t now;
static int mode;
static int launch(void *ctx) {
    init_service_runtime_t *sr = ctx;
    unsigned id = sr->desc->id;
    if (id == INIT_SVC_PROCESS_MANAGER) assert(events[INIT_SVC_NAMESVC] == 2);
    launched[id]++;
    sr->launch.event_receive_cap = id;
    sr->launch.service_send_cap = id + 10;
    return mode == 3 ? -EIO : 0;
}
static const init_service_id_t pm_deps[] = {INIT_SVC_NAMESVC};
const init_service_desc_t g_init_manifest[] = {
    {.id = INIT_SVC_NAMESVC, .name = "namesvc", .boot_class = BOOT_CLASS_CORE,
     .start_fn = launch, .policy = INIT_SERVICE_REQUIRED, .ready_deadline_ms = 5,
     .profile_mask = BHARAT_INIT_PROFILE_DESKTOP, .board_mask = BHARAT_INIT_BOARD_ANY,
     .personality_mask = BHARAT_INIT_PERSONALITY_ANY},
    {.id = INIT_SVC_PROCESS_MANAGER, .name = "process_manager", .boot_class = BOOT_CLASS_CORE,
     .start_fn = launch, .policy = INIT_SERVICE_REQUIRED, .ready_deadline_ms = 5,
     .profile_mask = BHARAT_INIT_PROFILE_DESKTOP, .board_mask = BHARAT_INIT_BOARD_ANY,
     .personality_mask = BHARAT_INIT_PERSONALITY_ANY, .deps = pm_deps, .dep_count = 1}
};
const size_t g_init_manifest_count = 2;
void bharat_runtime_log(const char *msg) { (void)msg; }
int bharat_sched_yield(void) { now += 1000000; return 0; }
int bharat_runtime_now_ns(uint64_t *out) { *out = now; return 0; }
int init_handoff_to_supervisor(const init_boot_context_t *ctx, struct init_runtime_s *rt) {
    (void)ctx; (void)rt; assert(0); return -EIO;
}
int bharat_bootstrap_poll(uint32_t cap, bh_bootstrap_service_event_t *event) {
    if (mode == 1) return BH_ERR_TRY_AGAIN; /* no fabricated readiness */
    if (events[cap] >= 2) return BH_ERR_TRY_AGAIN;
    *event = (bh_bootstrap_service_event_t){.version = BH_BOOTSTRAP_SERVICE_ABI,
        .service_id = cap, .type = ++events[cap] == 1 ? BH_BOOTSTRAP_EVENT_BOUND : BH_BOOTSTRAP_EVENT_READY};
    if (mode == 2) event->service_id = 99;
    return 0;
}
int main(void) {
    for (mode = 0; mode < 4; ++mode) {
        events[1] = events[2] = launched[1] = launched[2] = 0;
        now = 0;
        init_boot_context_t ctx = {.profile = INIT_PROFILE_DESKTOP};
        int result = init_runtime_run(&ctx);
        if (mode == 0) {
            assert(result == INIT_RUNTIME_RETAINED);
            assert(!ctx.safe_mode_requested && launched[1] == 1 && launched[2] == 1);
        } else {
            assert(result < 0 && ctx.safe_mode_requested && launched[2] == 0);
        }
    }
    init_service_runtime_t sr = {.desc = &g_init_manifest[0], .state = INIT_SERVICE_STATE_SPAWNED};
    bh_bootstrap_service_event_t event = {.version = 1, .service_id = 1, .type = BH_BOOTSTRAP_EVENT_READY};
    assert(init_service_apply_event(&sr, &event) < 0 && !sr.observed_ready);
    event.type = BH_BOOTSTRAP_EVENT_BOUND;
    event.version = 99;
    assert(init_service_apply_event(&sr, &event) < 0);
    event.version = 1;
    assert(init_service_apply_event(&sr, &event) == 0);
    event.type = BH_BOOTSTRAP_EVENT_READY;
    assert(init_service_apply_event(&sr, &event) == 0 && sr.observed_ready);
    printf("PASS: real event progression, dependencies, timeout, invalid sender/version, and launch failure\n");
    return 0;
}
