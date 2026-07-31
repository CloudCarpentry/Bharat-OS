#include <bharat/libc/alloc.h>
#include <standard/assert.h>
#include <standard/stddef.h>

static uint8_t g_pool_buf[1024];

int main(void) {
    bh_bounded_freelist_t fl;
    bh_bounded_freelist_init(&fl, g_pool_buf, sizeof(g_pool_buf));

    /* Test alloc */
    void *p1 = bh_bounded_freelist_alloc(&fl, 32);
    assert(p1 != NULL);
    assert(((uintptr_t)p1 & 7) == 0);

    void *p2 = bh_bounded_freelist_alloc(&fl, 64);
    assert(p2 != NULL);
    assert(p2 != p1);

    /* Test free & coalesce */
    bh_bounded_freelist_free(&fl, p1);
    bh_bounded_freelist_free(&fl, p2);

    /* Allocation of the total coalesced block size should now succeed */
    void *p3 = bh_bounded_freelist_alloc(&fl, 150);
    assert(p3 != NULL);

    return 0;
}
