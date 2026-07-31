#include <bharat/libc/alloc.h>
#include <standard/assert.h>
#include <standard/stddef.h>

static uint8_t g_arena_buf[64];

int main(void) {
    bh_fixed_arena_t arena;
    bh_fixed_arena_init(&arena, g_arena_buf, sizeof(g_arena_buf));

    /* Allocate almost up to capacity */
    void *p1 = bh_fixed_arena_alloc(&arena, 50, 8);
    assert(p1 != NULL);

    /* Allocate more, should fail and return NULL (exhausted) */
    void *p2 = bh_fixed_arena_alloc(&arena, 20, 8);
    assert(p2 == NULL);

    /* After reset, should work again */
    bh_fixed_arena_reset(&arena);
    void *p3 = bh_fixed_arena_alloc(&arena, 20, 8);
    assert(p3 != NULL);

    return 0;
}
