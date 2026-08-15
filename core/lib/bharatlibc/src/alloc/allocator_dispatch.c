#include <bharat/bsys/backend.h>
#include <bharat/libc/alloc.h>
#include <standard/stdint.h>

static bh_fixed_arena_t *g_fixed_arena = NULL;
static bh_bounded_freelist_t *g_bounded_freelist = NULL;
static bh_bounded_freelist_t g_backend_freelist;
static int g_backend_heap_initialized = 0;

void bh_allocator_set_fixed_arena(bh_fixed_arena_t *arena) {
  g_fixed_arena = arena;
  g_bounded_freelist = NULL;
}

void bh_allocator_set_bounded_freelist(bh_bounded_freelist_t *fl) {
  g_bounded_freelist = fl;
  g_fixed_arena = NULL;
}

void *bh_malloc(size_t size) {
  if (!g_bounded_freelist && !g_fixed_arena && !g_backend_heap_initialized) {
    const bh_bsys_backend_ops_t *backend = bh_bsys_get_backend();
    uintptr_t base = 0;
    uint32_t capacity = 0;

    /* Phase A is single-threaded. This process-local lazy initialization
     * must become synchronized before the pthread phase is enabled. */
    g_backend_heap_initialized = 1;
    if (backend && backend->heap_region &&
        backend->heap_region(&base, &capacity) == 0 && base != 0 &&
        (base & (sizeof(uint64_t) - 1U)) == 0U) {
      bh_bounded_freelist_init(&g_backend_freelist, (void *)base, capacity);
      g_bounded_freelist = &g_backend_freelist;
    }
  }
  if (g_bounded_freelist) {
    return bh_bounded_freelist_alloc(g_bounded_freelist, size);
  }
  if (g_fixed_arena) {
    return bh_fixed_arena_alloc(g_fixed_arena, size, 8);
  }
  return NULL;
}

void *malloc(size_t size) { return bh_malloc(size); }

void free(void *pointer) { bh_free(pointer); }

void bh_free(void *ptr) {
  if (g_bounded_freelist && ptr) {
    bh_bounded_freelist_free(g_bounded_freelist, ptr);
  }
}
