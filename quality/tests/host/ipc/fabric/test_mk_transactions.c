#include <assert.h>
#include <stdio.h>
#include "ipc/mk_proto.h"
#include "fake_hal.h"

int main(void) {
    printf("Running test_mk_transactions...\n");

    kstatus_t st = bh_mk_fabric_init(2);
    assert(st == K_OK);

    fake_hal_set_cpu_id(0);

    // 1. Allocate a transaction
    bh_mk_tx_handle_t handle1;
    st = bh_mk_tx_alloc(1, 10, 1, 2, 1000, &handle1);
    assert(st == K_OK);
    assert(handle1.slot < BH_MK_TX_TABLE_SIZE);

    // 2. Try to complete it with incorrect source core (should fail with DENIED)
    st = bh_mk_tx_complete(handle1, 0, 10, K_OK); // Expected 1, passed 0
    assert(st == K_ERR_DENIED);

    // 3. Complete it with correct parameters (should succeed)
    st = bh_mk_tx_complete(handle1, 1, 10, K_OK);
    assert(st == K_OK);

    // 4. Reap the transaction
    kstatus_t result;
    st = bh_mk_tx_reap(handle1, &result);
    assert(st == K_OK);
    assert(result == K_OK);

    // Reaped transaction handle should now be STALE
    st = bh_mk_tx_reap(handle1, &result);
    assert(st == K_ERR_CAP_STALE);

    // 5. ABA Check: Re-allocate in the same slot
    bh_mk_tx_handle_t handle2;
    st = bh_mk_tx_alloc(1, 10, 1, 2, 1000, &handle2);
    assert(st == K_OK);
    assert(handle2.slot == handle1.slot); // Same slot reused
    assert(handle2.generation == handle1.generation + 1); // But generation has incremented!

    // Attempting to complete/reap with stale handle1 should be safely rejected
    st = bh_mk_tx_complete(handle1, 1, 10, K_OK);
    assert(st == K_ERR_CAP_STALE);

    printf("test_mk_transactions PASSED\n");
    return 0;
}
