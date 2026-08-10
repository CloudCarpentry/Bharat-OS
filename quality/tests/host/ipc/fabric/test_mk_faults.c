#include <assert.h>
#include <stdio.h>
#include "ipc/mk_proto.h"
#include "fake_hal.h"

int main(void) {
    printf("Running test_mk_faults...\n");

    kstatus_t st = bh_mk_fabric_init(2);
    assert(st == K_OK);

    bh_mk_core_fabric_t *f1 = bh_mk_get_core_fabric(1);
    assert(f1);

    // Pack a valid destination endpoint handle for core 1
    bh_mk_endpoint_handle_t dest;
    st = bh_mk_handle_pack(1, 1, 1, 1, &dest);
    assert(st == K_OK);

    // 1. Mark core 1 NOT-ready (offline)
    atomic_store_explicit(&f1->ready, 0, memory_order_release);

    fake_hal_set_cpu_id(0);
    // Send to offline core should fail with DEV_OFFLINE
    st = bh_mk_send(BH_MK_ENDPOINT_LEGACY, dest, 1, 1, BH_MK_LANE_NORMAL, NULL, 0, NULL);
    assert(st == K_ERR_DEV_OFFLINE);

    // Reset core 1 back to ready
    atomic_store_explicit(&f1->ready, 1, memory_order_release);

    // 2. Send to invalid (unbound) endpoint slot
    bh_mk_endpoint_handle_t invalid_dest;
    st = bh_mk_handle_pack(1, 1, 1, 63, &invalid_dest); // slot 63 is valid but unbound
    assert(st == K_OK);

    st = bh_mk_send(BH_MK_ENDPOINT_LEGACY, invalid_dest, 1, 1, BH_MK_LANE_NORMAL, NULL, 0, NULL);
    assert(st == K_OK); // Send places in queue successfully, but let's see what happens on dequeue/dispatch!

    // On core 1, drain local. Resolving invalid endpoint should fail internally and log diagnostics.
    fake_hal_set_cpu_id(1);
    st = bh_mk_drain_local(10);
    assert(st == K_OK);
    assert(f1->diagnostics.rx_errors == 1); // Logged as rx error

    printf("test_mk_faults PASSED\n");
    return 0;
}
