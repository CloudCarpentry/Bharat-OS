#include <assert.h>
#include <stdio.h>
#include "ipc/mk_proto.h"
#include "fake_hal.h"

int main(void) {
    printf("Running test_mk_credits...\n");

    // Initialize fabric
    kstatus_t st = bh_mk_fabric_init(2);
    assert(st == K_OK);

    bh_mk_core_fabric_t *f0 = bh_mk_get_core_fabric(0);
    bh_mk_core_fabric_t *f1 = bh_mk_get_core_fabric(1);
    assert(f0 && f1);

    // Bind an endpoint on destination core 1
    bh_mk_endpoint_handle_t ep1;
    bh_mk_endpoint_config_t config = {
        .handler_fn = (void*)1, // stub
        .message_class = 1,
        .opcode = 1
    };
    fake_hal_set_cpu_id(1);
    st = bh_mk_endpoint_bind(&config, &ep1);
    assert(st == K_OK);

    // Set CPU to core 0 for sending
    fake_hal_set_cpu_id(0);

    // Send messages from 0 to 1 until NORMAL lane is full
    uint32_t normal_capacity = BH_MK_LANE_NORMAL_CAP;
    for (uint32_t i = 0; i < normal_capacity; i++) {
        st = bh_mk_send(BH_MK_ENDPOINT_LEGACY, ep1, 1, 1, BH_MK_LANE_NORMAL, NULL, 0, NULL);
        assert(st == K_OK);
    }

    // Next NORMAL send should return WOULD_BLOCK because credits are exhausted
    st = bh_mk_send(BH_MK_ENDPOINT_LEGACY, ep1, 1, 1, BH_MK_LANE_NORMAL, NULL, 0, NULL);
    assert(st == K_ERR_WOULD_BLOCK);

    // CONTROL lane is independent and should still have credits!
    st = bh_mk_send(BH_MK_ENDPOINT_LEGACY, ep1, 1, 1, BH_MK_LANE_CONTROL, NULL, 0, NULL);
    assert(st == K_OK);

    // Drain core 1 NORMAL queue once to free up 1 credit
    fake_hal_set_cpu_id(1);
    bh_mk_wire_message_t msg;
    st = bh_mk_mpsc_ring_dequeue(&f1->normal_in, &msg);
    assert(st == K_OK);

    // Credit should be replenished
    fake_hal_set_cpu_id(0);
    st = bh_mk_send(BH_MK_ENDPOINT_LEGACY, ep1, 1, 1, BH_MK_LANE_NORMAL, NULL, 0, NULL);
    assert(st == K_OK);

    // Next one should block again
    st = bh_mk_send(BH_MK_ENDPOINT_LEGACY, ep1, 1, 1, BH_MK_LANE_NORMAL, NULL, 0, NULL);
    assert(st == K_ERR_WOULD_BLOCK);

    // Generation safety check: reset fabric 1 and increment generation
    fake_hal_set_cpu_id(1);
    atomic_store_explicit(&f1->generation, 2, memory_order_release);

    // Sending should now detect stale generation and return K_ERR_STALE (quarantined)
    fake_hal_set_cpu_id(0);
    st = bh_mk_send(BH_MK_ENDPOINT_LEGACY, ep1, 1, 1, BH_MK_LANE_NORMAL, NULL, 0, NULL);
    assert(st == K_ERR_STALE);

    printf("test_mk_credits PASSED\n");
    return 0;
}
