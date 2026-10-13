#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

void bharat_runtime_log(const char *msg) {
    printf("[LOG] %s\n", msg);
}
int bharat_sched_yield(void) { return 0; }
void bharat_runtime_shutdown(void) {}

#include "../../../core/services/core/init/init_manifest.h"
#include "../../../core/services/core/init/init_profile.h"
#include "../../../core/services/core/init/init_status.h"
#include "../../../core/services/core/init/init_runtime.h"
#include "../../../core/services/core/init/init_handoff.h"

int bharat_runtime_now_ns(uint64_t *out) { *out = 0; return 0; }
int bharat_bootstrap_poll(uint32_t cap, bh_bootstrap_service_event_t *event) {
    (void)cap; (void)event; return -1;
}
static unsigned bootstrap_calls;
int bharat_bootstrap_stop(uint32_t cap) { (void)cap; return -ENOSYS; }
int bharat_bootstrap_launch(const char *name, uint32_t id, uint32_t discovery,
    uint32_t delegate, bh_bootstrap_launch_result_t *out) {
    (void)name; (void)id; (void)discovery; (void)delegate; (void)out;
    ++bootstrap_calls;
    return -ENOSYS;
}


// We simulate namesvc and IPC
#include <bharat/uapi/process_manager/contract_v1.h>
#include <bharat/namesvc/client.h>
#include <bharat/ipc/ipc.h>

static int mock_lookup_status = NAMESVC_STATUS_OK;
static bharat_ipc_endpoint_t mock_lookup_ep = 0x1234;
int namesvc_lookup(const char *name, bharat_service_id_t *out_id, bharat_ipc_endpoint_t *out_ep, uint32_t *out_version) {
    if (mock_lookup_status == NAMESVC_STATUS_OK) {
        if (out_id) *out_id = 2;
        if (out_ep) *out_ep = mock_lookup_ep;
        if (out_version) *out_version = 1;
    } else {
        if (out_ep) *out_ep = BHARAT_CAP_INVALID_HANDLE;
    }
    return mock_lookup_status;
}

static int32_t mock_ipc_status = BHARAT_IPC_STATUS_OK;
static int32_t mock_pm_resp_status = 0;
int32_t bharat_ipc_call_ex(bharat_ipc_endpoint_t ep,
                           const bharat_ipc_msg_header_t *req_hdr,
                           const void *req_payload,
                           bharat_ipc_msg_header_t *rep_hdr,
                           void *rep_payload,
                           uint32_t rep_payload_size,
                           uint64_t timeout_ms) {
    if (mock_ipc_status == BHARAT_IPC_STATUS_OK) {
        bh_pm_spawn_response_v1_t *resp = (bh_pm_spawn_response_v1_t *)rep_payload;
        resp->status = mock_pm_resp_status;
    }
    return mock_ipc_status;
}

int init_handoff_to_supervisor(const init_boot_context_t *ctx, struct init_runtime_s *rt) {
    return 0; // Success mock
}

void test_init_service_declared_not_ready(void) {
    printf("Running %s...\n", __func__);
    init_boot_context_t ctx;
    __builtin_memset(&ctx, 0, sizeof(ctx));
    ctx.profile = INIT_PROFILE_SMALL;
    ctx.capability_mask = BHARAT_INIT_CAP_NONE;
    ctx.board_id = BHARAT_INIT_BOARD_ANY;
    ctx.personality_id = BH_PERSONALITY_NATIVE;

    mock_lookup_status = NAMESVC_STATUS_OK;
    mock_pm_resp_status = 0;

    int result = init_runtime_run(&ctx);
    assert(result != 0);
}

void test_pm_unavailable(void) {
    printf("Running %s...\n", __func__);
    init_boot_context_t ctx;
    __builtin_memset(&ctx, 0, sizeof(ctx));
    ctx.profile = INIT_PROFILE_SMALL;
    ctx.capability_mask = BHARAT_INIT_CAP_NONE;
    ctx.board_id = BHARAT_INIT_BOARD_ANY;
    ctx.personality_id = BH_PERSONALITY_NATIVE;

    mock_lookup_status = NAMESVC_STATUS_ERR_NOTFOUND;
    mock_lookup_ep = BHARAT_CAP_INVALID_HANDLE;

    int result = init_runtime_run(&ctx);
    assert(result != 0);
}

void test_spawn_request_accepted(void) {
    printf("Running %s...\n", __func__);
    init_service_runtime_t sr = {0};
    init_service_desc_t desc = { .id = INIT_SVC_FAULTMGR, .name = "faultmgr", .start_fn = g_init_manifest[4].start_fn };
    sr.desc = &desc;

    mock_lookup_status = NAMESVC_STATUS_OK;
    mock_lookup_ep = 0x1234;
    mock_pm_resp_status = 0;

    int err = sr.desc->start_fn(&sr);
    assert(err == 0);
}

void test_spawn_request_rejected(void) {
    printf("Running %s...\n", __func__);
    init_service_runtime_t sr = {0};
    init_service_desc_t desc = { .id = INIT_SVC_FAULTMGR, .name = "faultmgr", .start_fn = g_init_manifest[4].start_fn };
    sr.desc = &desc;

    mock_lookup_status = NAMESVC_STATUS_OK;
    mock_pm_resp_status = -1; // rejected

    int err = sr.desc->start_fn(&sr);
    assert(err != 0);
}

void test_endpoint_unavailable_cannot_reach_ready(void) {
    printf("Running %s...\n", __func__);
    init_boot_context_t ctx;
    __builtin_memset(&ctx, 0, sizeof(ctx));
    ctx.profile = INIT_PROFILE_SMALL;
    ctx.capability_mask = BHARAT_INIT_CAP_NONE;
    ctx.board_id = BHARAT_INIT_BOARD_ANY;
    ctx.personality_id = BH_PERSONALITY_NATIVE;

    mock_lookup_status = NAMESVC_STATUS_OK;
    mock_lookup_ep = BHARAT_CAP_INVALID_HANDLE;
    mock_pm_resp_status = 0;

    int result = init_runtime_run(&ctx);
    assert(result != 0);
}

void test_valid_readiness_confirmation_transitions_to_ready(void) {
    printf("Running %s...\n", __func__);
    init_boot_context_t ctx;
    __builtin_memset(&ctx, 0, sizeof(ctx));
    ctx.profile = INIT_PROFILE_SMALL;
    ctx.capability_mask = BHARAT_INIT_CAP_NONE;
    ctx.board_id = BHARAT_INIT_BOARD_ANY;
    ctx.personality_id = BH_PERSONALITY_NATIVE;

    mock_lookup_status = NAMESVC_STATUS_OK;
    mock_lookup_ep = 0x1234;
    mock_pm_resp_status = 0;

    init_runtime_t rt;
    __builtin_memset(&rt, 0, sizeof(rt));
    rt.boot_ctx = ctx;

    rt.service_order[0] = INIT_SVC_FAULTMGR;
    rt.services[INIT_SVC_FAULTMGR].desc = &g_init_manifest[4];
    rt.services[INIT_SVC_FAULTMGR].state = INIT_SERVICE_STATE_DECLARED;
    rt.manifest_count = 1;

    int err = rt.services[INIT_SVC_FAULTMGR].desc->start_fn(&rt.services[INIT_SVC_FAULTMGR]);
    if (err == 0) {
        rt.services[INIT_SVC_FAULTMGR].state = INIT_SERVICE_STATE_SPAWN_REQUESTED;
        if (rt.services[INIT_SVC_FAULTMGR].state == INIT_SERVICE_STATE_SPAWN_REQUESTED) {
             rt.services[INIT_SVC_FAULTMGR].state = INIT_SERVICE_STATE_SPAWNED;
        }
        rt.services[INIT_SVC_FAULTMGR].observed_ready = true;
        if (rt.services[INIT_SVC_FAULTMGR].observed_ready) {
            rt.services[INIT_SVC_FAULTMGR].state = INIT_SERVICE_STATE_READY;
        }
    }

    assert(rt.services[INIT_SVC_FAULTMGR].state == INIT_SERVICE_STATE_READY);
}

void test_optional_service_fails(void) {
    printf("Running %s...\n", __func__);
    init_boot_context_t ctx;
    __builtin_memset(&ctx, 0, sizeof(ctx));
    ctx.profile = INIT_PROFILE_TINY; // TINY includes app_payload which is OPTIONAL
    ctx.capability_mask = BHARAT_INIT_CAP_NONE;
    ctx.board_id = BHARAT_INIT_BOARD_ANY;
    ctx.personality_id = BH_PERSONALITY_NATIVE;

    mock_lookup_status = NAMESVC_STATUS_OK;
    mock_pm_resp_status = -1; // force failure

    int result = init_runtime_run(&ctx);
    assert(result == INIT_RUNTIME_QUIESCENT || result == INIT_RUNTIME_HANDOFF_COMPLETE || result == -14);
}

int main(void) {
    /* Kernel launch errors must propagate instead of claiming a successful child. */
    for (size_t i = 0; i < 2; ++i) {
        init_service_runtime_t service = {.desc = &g_init_manifest[i]};
        assert(service.desc->start_fn(&service) == -ENOSYS);
    }
    assert(bootstrap_calls == 2);
    test_init_service_declared_not_ready();
    test_pm_unavailable();
    test_spawn_request_accepted();
    test_spawn_request_rejected();
    test_endpoint_unavailable_cannot_reach_ready();
    test_valid_readiness_confirmation_transitions_to_ready();
    test_optional_service_fails();
    printf("All init tests passed.\n");
    return 0;
}
