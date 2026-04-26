#include <assert.h>
#include <bharat/kernel/ds/bh_id_allocator.h>
#include <kernel/status.h>
#include <string.h>

void test_id_allocator_basic() {
    bh_id_allocator_t alloc;
    uint8_t bitmap[2]; // 16 bits
    uint32_t capacity = 10;

    assert(bh_id_allocator_init(&alloc, bitmap, capacity) == K_OK);
    assert(bh_id_allocator_capacity(&alloc) == capacity);

    uint32_t id1, id2, id3;
    assert(bh_id_allocator_alloc(&alloc, &id1) == K_OK);
    assert(id1 == 0);
    assert(bh_id_allocator_is_allocated(&alloc, id1) == true);

    assert(bh_id_allocator_alloc(&alloc, &id2) == K_OK);
    assert(id2 == 1);
    assert(bh_id_allocator_is_allocated(&alloc, id2) == true);

    assert(bh_id_allocator_free(&alloc, id1) == K_OK);
    assert(bh_id_allocator_is_allocated(&alloc, id1) == false);

    assert(bh_id_allocator_alloc(&alloc, &id3) == K_OK);
    assert(id3 == 2); // next_hint was 2
}

void test_id_allocator_full() {
    bh_id_allocator_t alloc;
    uint8_t bitmap[1];
    uint32_t capacity = 4;
    uint32_t id;

    bh_id_allocator_init(&alloc, bitmap, capacity);

    for (int i = 0; i < 4; i++) {
        assert(bh_id_allocator_alloc(&alloc, &id) == K_OK);
    }

    assert(bh_id_allocator_alloc(&alloc, &id) == K_ERR_PMM_EXHAUSTED);

    assert(bh_id_allocator_free(&alloc, 0) == K_OK);
    assert(bh_id_allocator_alloc(&alloc, &id) == K_OK);
    assert(id == 0);
}

void test_id_allocator_invalid_free() {
    bh_id_allocator_t alloc;
    uint8_t bitmap[1];
    bh_id_allocator_init(&alloc, bitmap, 4);

    assert(bh_id_allocator_free(&alloc, 5) == K_ERR_INVALID_ARG);
    assert(bh_id_allocator_free(&alloc, 0) == K_ERR_ALREADY_EXISTS);
}

int main() {
    test_id_allocator_basic();
    test_id_allocator_full();
    test_id_allocator_invalid_free();
    return 0;
}
