#include <assert.h>
#include <bharat/kernel/ds/bh_ring.h>
#include <kernel/status.h>

void test_ring_basic() {
    bh_ring_t ring;
    uint32_t storage[4];
    bh_ring_init(&ring, storage, 4, sizeof(uint32_t));

    assert(bh_ring_is_empty(&ring));

    uint32_t val = 100;
    assert(bh_ring_push(&ring, &val) == K_OK);
    val = 200;
    assert(bh_ring_push(&ring, &val) == K_OK);
    val = 300;
    assert(bh_ring_push(&ring, &val) == K_OK);

    assert(bh_ring_is_full(&ring));
    val = 400;
    assert(bh_ring_push(&ring, &val) == K_ERR_IPC_QUEUE_FULL);

    uint32_t out;
    assert(bh_ring_pop(&ring, &out) == K_OK);
    assert(out == 100);
    assert(bh_ring_pop(&ring, &out) == K_OK);
    assert(out == 200);

    val = 500;
    assert(bh_ring_push(&ring, &val) == K_OK);
    assert(bh_ring_push(&ring, &val) == K_OK);
    assert(bh_ring_is_full(&ring));

    assert(bh_ring_pop(&ring, &out) == K_OK);
    assert(out == 300);
    assert(bh_ring_pop(&ring, &out) == K_OK);
    assert(out == 500);
    assert(bh_ring_pop(&ring, &out) == K_OK);
    assert(out == 500);
    assert(bh_ring_is_empty(&ring));
}

int main() {
    test_ring_basic();
    return 0;
}
