#include <bharat/libc/alloc.h>

static bh_fixed_arena_t *g_fixed_arena = NULL;
static bh_bounded_freelist_t *g_bounded_freelist = NULL;

void bh_allocator_set_fixed_arena(bh_fixed_arena_t *arena) {
    g_fixed_arena = arena;
    g_bounded_freelist = NULL;
}

void bh_allocator_set_bounded_freelist(bh_bounded_freelist_t *fl) {
    g_bounded_freelist = fl;
    g_fixed_arena = NULL;
}

void *bh_malloc(size_t size) {
    if (g_bounded_freelist) {
        return bh_bounded_freelist_alloc(g_bounded_freelist, size);
    }
    if (g_fixed_arena) {
        return bh_fixed_arena_alloc(g_fixed_arena, size, 8);
    }
    return NULL;
}

void bh_free(void *ptr) {
    if (g_bounded_freelist && ptr) {
        bh_bounded_freelist_free(g_bounded_freelist, ptr);
    }
}
