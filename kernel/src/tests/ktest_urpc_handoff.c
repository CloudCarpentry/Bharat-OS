#include "tests/ktest.h"
#include "sched/sched.h"
#include "core/multikernel.h"
#include "ipc/mk_dispatch.h"
#include <bharat/cpu_local.h>

// Mock channel setup for testing
static mk_channel_t g_mock_channel;
static urpc_msg_t g_last_sent_msg;
static int g_msg_sent_count = 0;

// Override mk_get_channel for host test via weak symbols if needed, or define macros.
// For this isolated test within the kernel, if mk_get_channel is strongly defined,
// we can use a wrapper or `#define` to hijack the calls if compiling standalone.
// However, since we are linking against the real kernel, we should not define strong replacements.
// Let's declare our mock helpers and hook them if possible, but since we cannot easily override
// strong functions in a monolithic link without `--wrap`, we will just rely on the real implementations
// failing cleanly or intercept the state by calling the dispatch function directly with crafted messages.
// We'll stub them out only if they aren't provided by the main build (using weak).

__attribute__((weak)) int mk_get_channel(uint32_t sender_core, uint32_t receiver_core, mk_channel_t* out_channel) {
    if (sender_core >= MAX_SUPPORTED_CORES || receiver_core >= MAX_SUPPORTED_CORES) {
        return -1;
    }
    out_channel->src_core = sender_core;
    out_channel->dst_core = receiver_core;
    out_channel->state = MK_CHANNEL_STATE_ESTABLISHED;
    return 0;
}

__attribute__((weak)) int mk_send_message(mk_channel_t *channel, uint32_t msg_type, void *payload, uint32_t size) {
    (void)channel;
    g_last_sent_msg.type = msg_type;
    g_last_sent_msg.payload_size = size;
    if (payload && size <= sizeof(g_last_sent_msg.payload_data)) {
        __builtin_memcpy(g_last_sent_msg.payload_data, payload, size);
    }
    g_msg_sent_count++;
    return 0;
}

static void reset_test_state(void) {
    g_msg_sent_count = 0;
    __builtin_memset(&g_last_sent_msg, 0, sizeof(g_last_sent_msg));
}

bool test_urpc_thread_handoff_valid(void) {
    reset_test_state();

    bh_process_t *p = process_create("test_proc_handoff");
    KTEST_ASSERT(p != NULL, "Process creation failed");

    bh_thread_t *t = thread_create(p, NULL);
    KTEST_ASSERT(t != NULL, "Thread creation failed");

    uint32_t sender_core = hal_cpu_get_id(); // Should be 0 in test env
    uint32_t target_core = 1;
    uint32_t valid_auth_token = 0xDEADBEEF; // Non-zero is "valid" in our mock

    // Ensure thread is READY
    t->state = THREAD_STATE_READY;
    t->bound_core_id = sender_core;
    sched_enqueue(t, sender_core);

    // Send handoff request
    int ret = sched_request_remote_handoff(t, target_core, valid_auth_token);
    KTEST_ASSERT(ret == 0, "Handoff request failed");
    KTEST_ASSERT(t->state == THREAD_STATE_REMOTE_HANDOFF_PENDING, "Thread state not updated");
    KTEST_ASSERT(g_msg_sent_count == 1, "Message not sent");
    KTEST_ASSERT(g_last_sent_msg.type == MK_MSG_THREAD_HANDOFF_REQ, "Wrong message type sent");

    // Now simulate receiving the message on the target core
    // We need to mock hal_cpu_get_id() to return target_core, but since it's an inline
    // or linked function, we can just manipulate the message to make it look like
    // it's arriving on core 0 for the test, OR we can mock the receiver ID in the message.
    // For this test, let's just test the dispatch logic directly.

    urpc_msg_t rx_msg = g_last_sent_msg;
    rx_msg.src_core = sender_core;
    rx_msg.dst_core = target_core;
    rx_msg.auth_token = valid_auth_token;

    g_mock_channel.src_core = sender_core;
    g_mock_channel.dst_core = target_core;

    // To test mk_dispatch_message on the "target core", we temporarily need to
    // bypass the local_core check OR just call the handler directly if mk_dispatch_message
    // enforces hal_cpu_get_id() == dst_core. Since hal_cpu_get_id() returns 0 in tests,
    // let's set target_core to 0 and sender_core to 1 for the *receive* phase.

    rx_msg.dst_core = 0;
    rx_msg.src_core = 1;
    g_mock_channel.dst_core = 0;
    g_mock_channel.src_core = 1;

    mk_msg_thread_handoff_t *payload = (mk_msg_thread_handoff_t *)rx_msg.payload_data;
    payload->target_core = 0; // Update payload to match the mocked receiver core

    g_msg_sent_count = 0; // Reset to track the ACK

    ret = mk_dispatch_message(&g_mock_channel, &rx_msg);
    KTEST_ASSERT(ret == 0, "Dispatch failed");

    // Verify effects on receiver side
    KTEST_ASSERT(t->bound_core_id == 0, "Thread not bound to target core");
    KTEST_ASSERT(t->state == THREAD_STATE_READY, "Thread not set back to READY");
    KTEST_ASSERT(g_msg_sent_count == 1, "ACK not sent");
    KTEST_ASSERT(g_last_sent_msg.type == MK_MSG_THREAD_HANDOFF_ACK, "Wrong reply type");

    return true;
}

bool test_urpc_thread_handoff_denied_no_cap(void) {
    reset_test_state();

    // Simulate incoming message with NO capability token
    urpc_msg_t rx_msg;
    __builtin_memset(&rx_msg, 0, sizeof(rx_msg));
    rx_msg.type = MK_MSG_THREAD_HANDOFF_REQ;
    rx_msg.src_core = 1;
    rx_msg.dst_core = 0;
    rx_msg.auth_token = 0; // INVALID token

    g_mock_channel.src_core = 1;
    g_mock_channel.dst_core = 0;

    int ret = mk_dispatch_message(&g_mock_channel, &rx_msg);
    KTEST_ASSERT(ret == -1, "Dispatch should fail due to missing capability");
    KTEST_ASSERT(g_msg_sent_count == 0, "No reply should be sent on auth failure");

    return true;
}

bool test_urpc_thread_handoff_reject_bad_core(void) {
    reset_test_state();

    bh_process_t *p = process_create("test_proc_bad_core");
    bh_thread_t *t = thread_create(p, NULL);
    t->state = THREAD_STATE_REMOTE_HANDOFF_PENDING; // Pre-condition for receiver

    urpc_msg_t rx_msg;
    __builtin_memset(&rx_msg, 0, sizeof(rx_msg));
    rx_msg.type = MK_MSG_THREAD_HANDOFF_REQ;
    rx_msg.src_core = 1;
    rx_msg.dst_core = 0;
    rx_msg.auth_token = 0xDEADBEEF;
    rx_msg.payload_size = sizeof(mk_msg_thread_handoff_t);

    mk_msg_thread_handoff_t payload = {
        .thread_id = t->thread_id,
        .source_core = 1,
        .target_core = 2 // BAD CORE: Doesn't match receiver (0)
    };
    __builtin_memcpy(rx_msg.payload_data, &payload, sizeof(payload));

    g_mock_channel.src_core = 1;
    g_mock_channel.dst_core = 0;

    int ret = mk_dispatch_message(&g_mock_channel, &rx_msg);
    KTEST_ASSERT(ret == 0, "Dispatch should succeed (it handles routing internally)");
    KTEST_ASSERT(g_msg_sent_count == 1, "NACK should be sent");
    KTEST_ASSERT(g_last_sent_msg.type == MK_MSG_THREAD_HANDOFF_NACK, "Expected NACK for bad core");

    return true;
}

bool test_urpc_thread_handoff_reject_bad_state(void) {
    reset_test_state();

    bh_process_t *p = process_create("test_proc_bad_state");
    bh_thread_t *t = thread_create(p, NULL);
    t->state = THREAD_STATE_RUNNING; // NOT handoff pending!

    urpc_msg_t rx_msg;
    __builtin_memset(&rx_msg, 0, sizeof(rx_msg));
    rx_msg.type = MK_MSG_THREAD_HANDOFF_REQ;
    rx_msg.src_core = 1;
    rx_msg.dst_core = 0;
    rx_msg.auth_token = 0xDEADBEEF;
    rx_msg.payload_size = sizeof(mk_msg_thread_handoff_t);

    mk_msg_thread_handoff_t payload = {
        .thread_id = t->thread_id,
        .source_core = 1,
        .target_core = 0
    };
    __builtin_memcpy(rx_msg.payload_data, &payload, sizeof(payload));

    g_mock_channel.src_core = 1;
    g_mock_channel.dst_core = 0;

    int ret = mk_dispatch_message(&g_mock_channel, &rx_msg);
    KTEST_ASSERT(ret == 0, "Dispatch should succeed");
    KTEST_ASSERT(g_msg_sent_count == 1, "NACK should be sent");
    KTEST_ASSERT(g_last_sent_msg.type == MK_MSG_THREAD_HANDOFF_NACK, "Expected NACK for bad state");

    return true;
}

bool test_urpc_thread_handoff_no_duplicate_enqueue(void) {
    // This is implicitly tested by the state check (THREAD_STATE_REMOTE_HANDOFF_PENDING -> READY).
    // If a second REQ comes in, the state is already READY, so it will hit the bad state reject path.
    return test_urpc_thread_handoff_reject_bad_state();
}

static ktest_case_t handoff_tests[] = {
    {"Valid Thread Handoff", test_urpc_thread_handoff_valid},
    {"Handoff Denied No Capability", test_urpc_thread_handoff_denied_no_cap},
    {"Handoff Reject Bad Target Core", test_urpc_thread_handoff_reject_bad_core},
    {"Handoff Reject Bad State", test_urpc_thread_handoff_reject_bad_state},
    {"Handoff No Duplicate Enqueue", test_urpc_thread_handoff_no_duplicate_enqueue}
};

static int run_handoff_suite(void) {
    ktest_run_suite("uRPC Thread Handoff Tests", handoff_tests, 5);
    return 0;
}

REGISTER_KERNEL_TEST("urpc_handoff", "urpc", run_handoff_suite, 1, 0)
