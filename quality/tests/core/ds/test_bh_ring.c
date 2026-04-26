#include <bharat/kernel/ds/bh_ring.h>
#include <assert.h>
#include <stdio.h>
#include <stdint.h>

void test_ring_basic() {
    uint32_t storage[4]; // capacity 4
    bh_ring_t ring;
    bh_ring_init(&ring, storage, 4, sizeof(uint32_t));

    assert(bh_ring_is_empty(&ring));
    assert(!bh_ring_is_full(&ring));

    uint32_t val = 42;
    kstatus_t status = bh_ring_push(&ring, &val);
    assert(status == K_OK);
    assert(!bh_ring_is_empty(&ring));

    uint32_t out_val = 0;
    status = bh_ring_pop(&ring, &out_val);
    assert(status == K_OK);
    assert(out_val == 42);
    assert(bh_ring_is_empty(&ring));

    // Fill the ring. Note: ring capacity is N-1 due to head/tail logic in some implementations.
    // Let's check bh_ring.c logic again: ((ring->head + 1) % ring->capacity) == ring->tail
    // So with capacity 4, it can hold 3 elements.
    uint32_t i;
    for (i = 1; i <= 3; i++) {
        assert(bh_ring_push(&ring, &i) == K_OK);
    }
    assert(bh_ring_is_full(&ring));
    assert(bh_ring_push(&ring, &i) == K_ERR_IPC_QUEUE_FULL);

    // Pop and wraparound
    assert(bh_ring_pop(&ring, &out_val) == K_OK);
    assert(out_val == 1);
    assert(!bh_ring_is_full(&ring));

    uint32_t wrap_val = 100;
    assert(bh_ring_push(&ring, &wrap_val) == K_OK);
    assert(bh_ring_is_full(&ring));

    assert(bh_ring_pop(&ring, &out_val) == K_OK);
    assert(out_val == 2);
    assert(bh_ring_pop(&ring, &out_val) == K_OK);
    assert(out_val == 3);
    assert(bh_ring_pop(&ring, &out_val) == K_OK);
    assert(out_val == 100);
    assert(bh_ring_is_empty(&ring));

    printf("test_ring_basic passed\n");
}

int main() {
    test_ring_basic();
    return 0;
}
