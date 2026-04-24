#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "../../kernel/include/ipc/ipc_profile_policy.h"

#ifndef EXPECT_PREFER_URPC
#error "EXPECT_PREFER_URPC must be defined"
#endif
#ifndef EXPECT_ALLOW_BULK_IPC
#error "EXPECT_ALLOW_BULK_IPC must be defined"
#endif
#ifndef EXPECT_URPC_MAX
#error "EXPECT_URPC_MAX must be defined"
#endif
#ifndef EXPECT_CONTROL_TRANSPORT
#error "EXPECT_CONTROL_TRANSPORT must be defined"
#endif
#ifndef EXPECT_BULK_TRANSPORT
#error "EXPECT_BULK_TRANSPORT must be defined"
#endif

static void test_policy_basics(void) {
    ipc_profile_policy_t policy = ipc_profile_policy_current();

    assert(policy.endpoint_payload_max == BHARAT_IPC_ENDPOINT_PAYLOAD_MAX);
    assert(policy.max_endpoints == BHARAT_IPC_MAX_ENDPOINTS);
    assert(policy.urpc_ring_size == URPC_RING_SIZE);
    assert(policy.prefer_urpc_for_control == (EXPECT_PREFER_URPC != 0));
    assert(policy.allow_bulk_ipc == (EXPECT_ALLOW_BULK_IPC != 0));
    assert(policy.max_cross_core_payload == EXPECT_URPC_MAX);
}

static void test_transport_selection(void) {
    assert(ipc_profile_select_transport(IPC_TRAFFIC_CONTROL, true) == EXPECT_CONTROL_TRANSPORT);
    assert(ipc_profile_select_transport(IPC_TRAFFIC_BULK, true) == EXPECT_BULK_TRANSPORT);
    assert(ipc_profile_select_transport(IPC_TRAFFIC_SERVICE, false) == IPC_TRANSPORT_ENDPOINT);
}

static void test_payload_rules(void) {
    assert(ipc_profile_payload_supported(IPC_TRAFFIC_CONTROL, 1U, true));
    assert(!ipc_profile_payload_supported(IPC_TRAFFIC_CONTROL, 0U, true));

    if (EXPECT_CONTROL_TRANSPORT == IPC_TRANSPORT_URPC) {
        assert(ipc_profile_payload_supported(IPC_TRAFFIC_CONTROL, EXPECT_URPC_MAX, true));
        assert(!ipc_profile_payload_supported(IPC_TRAFFIC_CONTROL, (uint32_t)EXPECT_URPC_MAX + 1U, true));
    } else {
        assert(ipc_profile_payload_supported(IPC_TRAFFIC_CONTROL, BHARAT_IPC_ENDPOINT_PAYLOAD_MAX, true));
    }
}

int main(void) {
    test_policy_basics();
    test_transport_selection();
    test_payload_rules();

    printf("test_ipc_urpc_policy_matrix passed\n");
    return 0;
}
