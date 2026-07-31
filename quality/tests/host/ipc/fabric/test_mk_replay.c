#include <assert.h>
#include <stdio.h>
#include "ipc/mk_proto.h"
#include "fake_hal.h"

int main(void) {
    printf("Running test_mk_replay...\n");

    kstatus_t st = bh_mk_fabric_init(1);
    assert(st == K_OK);

    fake_hal_set_cpu_id(0);

    // 1. First time receiving sequence 100 from core 1/endpoint 10 should be OK
    st = bh_mk_replay_check_and_add(1, 10, 1, 0, 0, 1, 1, 100, 1000);
    assert(st == K_OK);

    // 2. Duplicate receive should be rejected with ALREADY_EXISTS
    st = bh_mk_replay_check_and_add(1, 10, 1, 0, 0, 1, 1, 100, 1001);
    assert(st == K_ERR_ALREADY_EXISTS);

    // 3. Different sequence should be OK
    st = bh_mk_replay_check_and_add(1, 10, 1, 0, 0, 1, 1, 101, 1002);
    assert(st == K_OK);

    // 4. Fill replay cache to test circular overwrite
    // Cache size is BH_MK_REPLAY_CACHE_SIZE (64)
    // We do 70 inserts to guarantee wrap-around and overwrite seq 1 (which will be at index 2)
    for (uint64_t seq = 1; seq <= 70; seq++) {
        st = bh_mk_replay_check_and_add(2, 20, 1, 0, 0, 1, 1, seq, 2000 + seq);
        assert(st == K_OK);
    }

    // Now, seq 1 has been overwritten. A duplicate check for seq 1 should succeed!
    st = bh_mk_replay_check_and_add(2, 20, 1, 0, 0, 1, 1, 1, 3000);
    assert(st == K_OK);

    // But seq 70 is still in cache, so it should be rejected
    st = bh_mk_replay_check_and_add(2, 20, 1, 0, 0, 1, 1, 70, 3001);
    assert(st == K_ERR_ALREADY_EXISTS);

    printf("test_mk_replay PASSED\n");
    return 0;
}
