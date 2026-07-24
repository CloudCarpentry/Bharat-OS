#ifndef BHARAT_KERNEL_DS_INTRUSIVE_LIST_H
#define BHARAT_KERNEL_DS_INTRUSIVE_LIST_H

#include <stddef.h>
#include <stdbool.h>

/**
 * @file bh_intrusive_list.h
 * @brief Kernel Intrusive Doubly-Linked List
 *
 * This provides a standard intrusive list similar to the Linux kernel list.h.
 */

typedef struct bh_list_node {
    struct bh_list_node *next;
    struct bh_list_node *prev;
} bh_list_node_t;

#define BH_LIST_HEAD_INIT(name) { &(name), &(name) }

#define BH_LIST_HEAD(name) \
    bh_list_node_t name = BH_LIST_HEAD_INIT(name)

static inline void bh_list_init(bh_list_node_t *head) {
    head->next = head;
    head->prev = head;
}

static inline void bh_list_add(bh_list_node_t *head, bh_list_node_t *node) {
    node->next = head->next;
    node->prev = head;
    head->next->prev = node;
    head->next = node;
}

static inline void bh_list_add_tail(bh_list_node_t *head, bh_list_node_t *node) {
    node->next = head;
    node->prev = head->prev;
    head->prev->next = node;
    head->prev = node;
}

static inline void bh_list_del(bh_list_node_t *node) {
    node->next->prev = node->prev;
    node->prev->next = node->next;
    node->next = NULL;
    node->prev = NULL;
}

static inline bool bh_list_is_empty(const bh_list_node_t *head) {
    return head->next == head;
}

#define bh_list_entry(ptr, type, member) \
    ((type *)((char *)(ptr) - offsetof(type, member)))

#define bh_list_foreach(pos, head) \
    for (pos = (head)->next; pos != (head); pos = pos->next)

#define bh_list_foreach_safe(pos, n, head) \
    for (pos = (head)->next, n = pos->next; pos != (head); \
         pos = n, n = pos->next)

#endif // BHARAT_KERNEL_DS_INTRUSIVE_LIST_H
