#ifndef BHARAT_KERNEL_DS_BH_LIST_H
#define BHARAT_KERNEL_DS_BH_LIST_H

#include <stddef.h>
#include <stdbool.h>

/**
 * @file bh_list.h
 * @brief Canonical Kernel Intrusive Doubly-Linked List
 */

typedef struct bh_list_node {
    struct bh_list_node *next;
    struct bh_list_node *prev;
} bh_list_node_t;

#define BH_LIST_HEAD_INIT(name) { &(name), &(name) }

#define BH_LIST_HEAD(name) \
    bh_list_node_t name = BH_LIST_HEAD_INIT(name)

#ifndef BH_CONTAINER_OF
#define BH_CONTAINER_OF(ptr, type, member) \
    ((type *)((char *)(ptr) - offsetof(type, member)))
#endif

static inline void bh_list_init(bh_list_node_t *head) {
    head->next = head;
    head->prev = head;
}

static inline bool bh_list_is_empty(const bh_list_node_t *head) {
    return head->next == head;
}

static inline void bh_list_insert_after(bh_list_node_t *prev, bh_list_node_t *node) {
    node->next = prev->next;
    node->prev = prev;
    prev->next->prev = node;
    prev->next = node;
}

static inline void bh_list_insert_before(bh_list_node_t *next, bh_list_node_t *node) {
    node->prev = next->prev;
    node->next = next;
    next->prev->next = node;
    next->prev = node;
}

static inline void bh_list_push_front(bh_list_node_t *head, bh_list_node_t *node) {
    bh_list_insert_after(head, node);
}

static inline void bh_list_push_back(bh_list_node_t *head, bh_list_node_t *node) {
    bh_list_insert_before(head, node);
}

static inline void bh_list_remove(bh_list_node_t *node) {
    node->next->prev = node->prev;
    node->prev->next = node->next;
    node->next = NULL;
    node->prev = NULL;
}

static inline bh_list_node_t *bh_list_pop_front(bh_list_node_t *head) {
    if (bh_list_is_empty(head)) {
        return NULL;
    }
    bh_list_node_t *node = head->next;
    bh_list_remove(node);
    return node;
}

static inline bh_list_node_t *bh_list_pop_back(bh_list_node_t *head) {
    if (bh_list_is_empty(head)) {
        return NULL;
    }
    bh_list_node_t *node = head->prev;
    bh_list_remove(node);
    return node;
}

#define bh_list_entry(ptr, type, member) BH_CONTAINER_OF(ptr, type, member)

#define bh_list_foreach(pos, head) \
    for (pos = (head)->next; pos != (head); pos = pos->next)

#define bh_list_foreach_safe(pos, n, head) \
    for (pos = (head)->next, n = pos->next; pos != (head); \
         pos = n, n = pos->next)

#endif // BHARAT_KERNEL_DS_BH_LIST_H
