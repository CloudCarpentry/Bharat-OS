#include <assert.h>
#include <stddef.h>
#include <bharat/kernel/ds/bh_range_tree.h>
#include <kernel/status.h>

void test_range_tree_basic() {
    bh_range_tree_t tree;
    bh_range_t storage[10];
    bh_range_tree_init(&tree, storage, 10);

    assert(bh_range_tree_insert(&tree, 0x1000, 0x1000, (void*)1) == K_OK);
    assert(bh_range_tree_insert(&tree, 0x3000, 0x1000, (void*)2) == K_OK);
    assert(bh_range_tree_insert(&tree, 0x2000, 0x0800, (void*)3) == K_OK); // Middle insertion

    assert(tree.count == 3);
    assert(tree.ranges[0].start == 0x1000);
    assert(tree.ranges[1].start == 0x2000);
    assert(tree.ranges[2].start == 0x3000);

    bh_range_t res;
    assert(bh_range_tree_find_by_addr(&tree, 0x1500, &res) == K_OK);
    assert(res.data == (void*)1);

    assert(bh_range_tree_find_by_addr(&tree, 0x27FF, &res) == K_OK);
    assert(res.data == (void*)3);

    assert(bh_range_tree_find_by_addr(&tree, 0x2800, &res) == K_ERR_NOT_FOUND);
}

void test_range_tree_overlap() {
    bh_range_tree_t tree;
    bh_range_t storage[10];
    bh_range_tree_init(&tree, storage, 10);

    bh_range_tree_insert(&tree, 0x1000, 0x1000, NULL);
    assert(bh_range_tree_insert(&tree, 0x1800, 0x0800, NULL) == K_ERR_ALREADY_EXISTS);
    assert(bh_range_tree_insert(&tree, 0x0800, 0x0900, NULL) == K_ERR_ALREADY_EXISTS);
    assert(bh_range_tree_insert(&tree, 0x0800, 0x2000, NULL) == K_ERR_ALREADY_EXISTS);
}

void test_range_tree_remove() {
    bh_range_tree_t tree;
    bh_range_t storage[10];
    bh_range_tree_init(&tree, storage, 10);

    bh_range_tree_insert(&tree, 0x1000, 0x1000, NULL);
    bh_range_tree_insert(&tree, 0x2000, 0x1000, NULL);

    assert(bh_range_tree_remove(&tree, 0x1000) == K_OK);
    assert(tree.count == 1);
    assert(tree.ranges[0].start == 0x2000);

    assert(bh_range_tree_remove(&tree, 0x3000) == K_ERR_NOT_FOUND);
}

int main() {
    test_range_tree_basic();
    test_range_tree_overlap();
    test_range_tree_remove();
    return 0;
}
