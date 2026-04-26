#include <bharat/kernel/ds/bh_bitmap.h>
#include <assert.h>
#include <stdio.h>
#include <stdint.h>

void test_bitmap_basic() {
    uint8_t storage[2]; // 16 bits
    bh_bitmap_t bitmap;
    kstatus_t status = bh_bitmap_init(&bitmap, storage, 16);
    assert(status == K_OK);

    assert(!bh_bitmap_test(&bitmap, 0));
    bh_bitmap_set(&bitmap, 0);
    assert(bh_bitmap_test(&bitmap, 0));

    bh_bitmap_set(&bitmap, 10);
    assert(bh_bitmap_test(&bitmap, 10));
    assert(!bh_bitmap_test(&bitmap, 11));

    bh_bitmap_clear(&bitmap, 0);
    assert(!bh_bitmap_test(&bitmap, 0));

    size_t first_clear;
    status = bh_bitmap_find_first_clear(&bitmap, &first_clear);
    assert(status == K_OK);
    assert(first_clear == 0);

    size_t first_set;
    status = bh_bitmap_find_first_set(&bitmap, &first_set);
    assert(status == K_OK);
    assert(first_set == 10);

    bh_bitmap_set_range(&bitmap, 2, 4); // set 2, 3, 4, 5
    assert(bh_bitmap_test(&bitmap, 2));
    assert(bh_bitmap_test(&bitmap, 5));
    assert(!bh_bitmap_test(&bitmap, 6));

    bh_bitmap_clear_range(&bitmap, 10, 1);
    assert(!bh_bitmap_test(&bitmap, 10));

    // Test out of bounds
    assert(bh_bitmap_set(&bitmap, 16) == K_ERR_INVALID_ARG);
    assert(!bh_bitmap_test(&bitmap, 16));

    printf("test_bitmap_basic passed\n");
}

int main() {
    test_bitmap_basic();
    return 0;
}
