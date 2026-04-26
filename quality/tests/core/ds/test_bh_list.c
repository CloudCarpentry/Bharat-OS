#include <bharat/kernel/ds/bh_list.h>
#include <assert.h>
#include <stdio.h>

typedef struct {
    int value;
    bh_list_node_t node;
} test_item_t;

void test_list_basic() {
    BH_LIST_HEAD(head);
    assert(bh_list_is_empty(&head));

    test_item_t item1 = {.value = 1};
    test_item_t item2 = {.value = 2};
    test_item_t item3 = {.value = 3};

    bh_list_push_back(&head, &item1.node);
    assert(!bh_list_is_empty(&head));

    bh_list_push_front(&head, &item2.node);
    bh_list_push_back(&head, &item3.node);

    // List should be: item2 (2), item1 (1), item3 (3)
    bh_list_node_t *pos;
    int expected[] = {2, 1, 3};
    int i = 0;
    bh_list_foreach(pos, &head) {
        test_item_t *item = bh_list_entry(pos, test_item_t, node);
        assert(item->value == expected[i++]);
    }

    bh_list_remove(&item1.node);
    // List: item2 (2), item3 (3)
    assert(bh_list_entry(head.next, test_item_t, node)->value == 2);
    assert(bh_list_entry(head.prev, test_item_t, node)->value == 3);

    bh_list_node_t *popped = bh_list_pop_front(&head);
    assert(popped == &item2.node);
    assert(bh_list_entry(head.next, test_item_t, node)->value == 3);

    popped = bh_list_pop_back(&head);
    assert(popped == &item3.node);
    assert(bh_list_is_empty(&head));

    printf("test_list_basic passed\n");
}

int main() {
    test_list_basic();
    return 0;
}
