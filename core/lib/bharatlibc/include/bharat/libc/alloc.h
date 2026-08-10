#ifndef BHARATLIBC_ALLOC_H
#define BHARATLIBC_ALLOC_H

#include <standard/stddef.h>
#include <standard/stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 1. Fixed Arena Allocator */
typedef struct {
    uint8_t *buffer;
    size_t capacity;
    size_t offset;
} bh_fixed_arena_t;

void bh_fixed_arena_init(bh_fixed_arena_t *arena, void *buffer, size_t capacity);
void *bh_fixed_arena_alloc(bh_fixed_arena_t *arena, size_t size, size_t alignment);
void bh_fixed_arena_reset(bh_fixed_arena_t *arena);

/* 2. Bounded Freelist Allocator */
typedef struct bh_freelist_node {
    size_t size;
    struct bh_freelist_node *next;
} bh_freelist_node_t;

typedef struct {
    uint8_t *buffer;
    size_t capacity;
    bh_freelist_node_t *free_list;
} bh_bounded_freelist_t;

void bh_bounded_freelist_init(bh_bounded_freelist_t *fl, void *buffer, size_t capacity);
void *bh_bounded_freelist_alloc(bh_bounded_freelist_t *fl, size_t size);
void bh_bounded_freelist_free(bh_bounded_freelist_t *fl, void *ptr);

/* 3. Global Dispatch Allocator */
void bh_allocator_set_fixed_arena(bh_fixed_arena_t *arena);
void bh_allocator_set_bounded_freelist(bh_bounded_freelist_t *fl);

void *bh_malloc(size_t size);
void bh_free(void *ptr);

#ifdef __cplusplus
}
#endif

#endif /* BHARATLIBC_ALLOC_H */
