#ifndef BHARAT_RBTREE_H
#define BHARAT_RBTREE_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define RB_RED   0
#define RB_BLACK 1

struct rb_node {
    uint64_t __rb_parent_color;
    struct rb_node *rb_right;
    struct rb_node *rb_left;
} __attribute__((aligned(sizeof(long))));

struct rb_root {
    struct rb_node *rb_node;
};

#define rb_parent(r)   ((struct rb_node *)((r)->__rb_parent_color & ~3))
#define rb_color(r)   ((r)->__rb_parent_color & 1)
#define rb_is_red(r)   (!rb_color(r))
#define rb_is_black(r) rb_color(r)
#define rb_set_red(r)  do { (r)->__rb_parent_color &= ~1; } while (0)
#define rb_set_black(r)  do { (r)->__rb_parent_color |= 1; } while (0)

static inline void rb_set_parent(struct rb_node *rb, struct rb_node *p) {
    rb->__rb_parent_color = (rb->__rb_parent_color & 3) | (uintptr_t)p;
}

static inline void rb_set_parent_color(struct rb_node *rb, struct rb_node *p, int color) {
    rb->__rb_parent_color = (uintptr_t)p | color;
}

#define RB_ROOT (struct rb_root) { NULL, }

static inline void rb_link_node(struct rb_node *node, struct rb_node *parent,
                struct rb_node **rb_link) {
    node->__rb_parent_color = (uintptr_t)parent;
    node->rb_left = node->rb_right = NULL;
    *rb_link = node;
}

void rb_insert_color(struct rb_node *, struct rb_root *);
void rb_erase(struct rb_node *, struct rb_root *);
struct rb_node *rb_first(const struct rb_root *);
struct rb_node *rb_next(const struct rb_node *);

#endif /* BHARAT_RBTREE_H */
