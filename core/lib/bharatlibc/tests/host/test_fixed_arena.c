#include <bharat/libc/alloc.h>
#include <standard/assert.h>
#include <standard/stddef.h>

static uint8_t g_arena_buf[1024];

int main(void) {
    bh_fixed_arena_t arena;
    bh_fixed_arena_init(&arena, g_arena_buf, sizeof(g_arena_buf));

    /* Test simple aligned allocation */
    void *p1 = bh_fixed_arena_alloc(&arena, 10, 8);
    assert(p1 != NULL);
    assert(((uintptr_t)p1 & 7) == 0);

    void *p2 = bh_fixed_arena_alloc(&arena, 15, 4);
    assert(p2 != NULL);
    assert(((uintptr_t)p2 & 3) == 0);
    assert((uintptr_t)p2 >= (uintptr_t)p1 + 10);

    /* Test reset */
    bh_fixed_arena_reset(&arena);
    void *p3 = bh_fixed_arena_alloc(&arena, 10, 8);
    assert(p3 == p1); /* Reuses buffer start */

    return 0;
}
