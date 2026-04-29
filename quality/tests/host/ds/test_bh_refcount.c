#include "bharat/kernel/ds/bh_refcount.h"
#include <assert.h>
#include <stdio.h>
#include <stdint.h>

// Mock kernel_panic for host tests
void kernel_panic(const char *msg) {
    fprintf(stderr, "KERNEL PANIC: %s\n", msg);
    // In host tests, we might want to catch this, but for now just exit
    // Or we can use longjmp to test panic behavior
}

void test_refcount_basic() {
    bh_refcount_t ref;
    bh_refcount_init(&ref, 1);
    assert(bh_refcount_read(&ref) == 1);
    assert(!bh_refcount_is_zero(&ref));

    kstatus_t status = bh_refcount_try_inc(&ref);
    assert(status == K_OK);
    assert(bh_refcount_read(&ref) == 2);

    bool is_zero = false;
    status = bh_refcount_dec_and_test(&ref, &is_zero);
    assert(status == K_OK);
    assert(!is_zero);
    assert(bh_refcount_read(&ref) == 1);

    status = bh_refcount_dec_and_test(&ref, &is_zero);
    assert(status == K_OK);
    assert(is_zero);
    assert(bh_refcount_read(&ref) == 0);
    assert(bh_refcount_is_zero(&ref));

    printf("test_refcount_basic passed\n");
}

void test_refcount_no_resurrection() {
    bh_refcount_t ref;
    bh_refcount_init(&ref, 0);
    assert(bh_refcount_is_zero(&ref));

    kstatus_t status = bh_refcount_try_inc(&ref);
    assert(status == K_ERR_BAD_STATE);
    assert(bh_refcount_read(&ref) == 0);

    printf("test_refcount_no_resurrection passed\n");
}

void test_refcount_overflow() {
    bh_refcount_t ref;
    bh_refcount_init(&ref, UINT32_MAX);

    kstatus_t status = bh_refcount_try_inc(&ref);
    assert(status == K_ERR_OVERFLOW);
    assert(bh_refcount_read(&ref) == UINT32_MAX);

    printf("test_refcount_overflow passed\n");
}

void test_refcount_underflow() {
    bh_refcount_t ref;
    bh_refcount_init(&ref, 0);

    bool is_zero = false;
    kstatus_t status = bh_refcount_dec_and_test(&ref, &is_zero);
    assert(status == K_ERR_BAD_STATE);
    assert(bh_refcount_read(&ref) == 0);

    printf("test_refcount_underflow passed\n");
}

int main() {
    test_refcount_basic();
    test_refcount_no_resurrection();
    test_refcount_overflow();
    test_refcount_underflow();
    printf("All bh_refcount tests passed!\n");
    return 0;
}
