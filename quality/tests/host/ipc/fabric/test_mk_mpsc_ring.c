#include <assert.h>
#include <stdio.h>
#include "ipc/mk_proto.h"
#include "fake_hal.h"

// Functions to compile in
kstatus_t bh_mk_mpsc_ring_init(bh_mk_mpsc_ring_t *ring, bh_mk_ring_slot_t *slots, uint32_t capacity);
kstatus_t bh_mk_mpsc_ring_enqueue(bh_mk_mpsc_ring_t *ring, const bh_mk_wire_message_t *msg);
kstatus_t bh_mk_mpsc_ring_dequeue(bh_mk_mpsc_ring_t *ring, bh_mk_wire_message_t *out_msg);

int main(void) {
    printf("Running test_mk_mpsc_ring...\n");

    bh_mk_ring_slot_t slots[8];
    bh_mk_mpsc_ring_t ring;

    // 1. Initial State
    kstatus_t st = bh_mk_mpsc_ring_init(&ring, slots, 8);
    assert(st == K_OK);
    assert(ring.capacity == 8);
    assert(ring.consumer_tail == 0);
    assert(atomic_load(&ring.available_credits) == 8);

    // 2. Fill the queue
    for (int i = 0; i < 8; i++) {
        bh_mk_wire_message_t msg = {0};
        msg.header.sequence = i + 100;
        st = bh_mk_mpsc_ring_enqueue(&ring, &msg);
        assert(st == K_OK);
    }

    // Credits should be zero
    assert(atomic_load(&ring.available_credits) == 0);

    // Enqueue to a full queue should fail with WOULD_BLOCK
    bh_mk_wire_message_t fail_msg = {0};
    st = bh_mk_mpsc_ring_enqueue(&ring, &fail_msg);
    assert(st == K_ERR_WOULD_BLOCK);

    // 3. Dequeue items
    for (int i = 0; i < 8; i++) {
        bh_mk_wire_message_t out_msg;
        st = bh_mk_mpsc_ring_dequeue(&ring, &out_msg);
        assert(st == K_OK);
        assert(out_msg.header.sequence == (uint64_t)(i + 100));
    }

    // Credits should return to 8
    assert(atomic_load(&ring.available_credits) == 8);

    // Dequeue from empty queue should fail with AGAIN
    bh_mk_wire_message_t empty_msg;
    st = bh_mk_mpsc_ring_dequeue(&ring, &empty_msg);
    assert(st == K_ERR_AGAIN);

    printf("test_mk_mpsc_ring PASSED\n");
    return 0;
}
