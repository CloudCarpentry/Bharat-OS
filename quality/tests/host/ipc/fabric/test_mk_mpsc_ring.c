#include <assert.h>
#include <stdio.h>
#include <stdint.h>
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

    // Seed an empty ring immediately before uint32_t rollover.  Each slot's
    // free sequence must match its next producer ticket modulo 2^32.
    const uint32_t near_wrap = UINT32_MAX - 2U;
    atomic_store(&ring.producer_head, near_wrap);
    ring.consumer_tail = near_wrap;
    atomic_store(&ring.available_credits, ring.capacity);
    for (uint32_t i = 0; i < ring.capacity; ++i) {
        uint32_t ticket = near_wrap + i;
        atomic_store(&slots[ticket & ring.mask].sequence, ticket);
    }
    for (uint32_t i = 0; i < 5U; ++i) {
        bh_mk_wire_message_t msg = {0};
        msg.header.sequence = 200U + i;
        assert(bh_mk_mpsc_ring_enqueue(&ring, &msg) == K_OK);
        bh_mk_wire_message_t out_msg;
        assert(bh_mk_mpsc_ring_dequeue(&ring, &out_msg) == K_OK);
        assert(out_msg.header.sequence == 200U + i);
    }
    assert(atomic_load(&ring.producer_head) == 2U);
    assert(ring.consumer_tail == 2U);

    printf("test_mk_mpsc_ring PASSED\n");
    return 0;
}
