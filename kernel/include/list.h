// kernel/include/list.h
#ifndef BHARAT_LIST_H
#define BHARAT_LIST_H

#include <stddef.h>

typedef struct list_head {
    struct list_head *next, *prev;
} list_head_t;

static inline void list_init(list_head_t *list) {
    list->next = list;
    list->prev = list;
}

static inline void list_add(list_head_t *new_node, list_head_t *head) {
    new_node->next = head->next;
    new_node->prev = head;
    head->next->prev = new_node;
    head->next = new_node;
}

static inline void list_del(list_head_t *entry) {
    entry->next->prev = entry->prev;
    entry->prev->next = entry->next;
    entry->next = NULL;
    entry->prev = NULL;
}

static inline int list_empty(const list_head_t *head) {
    return head->next == head;
}

// Macro to resolve the struct containing this list node
#define list_entry(ptr, type, member) \
    ((type *)((char *)(ptr) - offsetof(type, member)))

#endif // BHARAT_LIST_H
