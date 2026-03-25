#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

// Mock functions to satisfy ramfs.c dependencies
void* kmalloc(size_t size) {
    return malloc(size);
}

void kfree(void* ptr) {
    free(ptr);
}

// vfs_register_driver is called in ramfs_register_driver
#include "fs/vfs.h"
int vfs_register_driver(const vfs_driver_info_t* info) {
    (void)info;
    return 0;
}

// Now include the source file to test the static function
#include "../../../kernel/src/fs/ramfs.c"

void test_ramfs_add_child_error_paths() {
    // 1. Test NULL parent
    ramfs_node_t child;
    memset(&child, 0, sizeof(child));
    child.vfs_node.flags = 1; // File
    assert(ramfs_add_child(NULL, &child) == -1);

    // 2. Test NULL child
    ramfs_node_t parent;
    memset(&parent, 0, sizeof(parent));
    parent.vfs_node.flags = 2; // Directory flag in ramfs.c
    assert(ramfs_add_child(&parent, NULL) == -1);

    // 3. Test non-directory parent (flags != 2)
    ramfs_node_t parent_file;
    memset(&parent_file, 0, sizeof(parent_file));
    parent_file.vfs_node.flags = 1; // File

    // Ensure side effects do not happen on failure
    parent_file.dir.child_count = 0;
    parent_file.dir.children = NULL;
    assert(ramfs_add_child(&parent_file, &child) == -1);
    assert(parent_file.dir.child_count == 0);
    assert(parent_file.dir.children == NULL);
}

void test_ramfs_add_child_success() {
    // 1. Test adding first child
    ramfs_node_t parent;
    memset(&parent, 0, sizeof(parent));
    parent.vfs_node.flags = 2; // Directory

    // Set up initial capacity (simulating ramfs_create_node for directory)
    parent.dir.child_capacity = 4;
    parent.dir.children = (ramfs_node_t **)kmalloc(sizeof(ramfs_node_t *) * parent.dir.child_capacity);
    assert(parent.dir.children != NULL);
    parent.dir.child_count = 0;

    ramfs_node_t child1;
    memset(&child1, 0, sizeof(child1));
    child1.vfs_node.flags = 1; // File

    assert(ramfs_add_child(&parent, &child1) == 0);
    assert(parent.dir.child_count == 1);
    assert(parent.dir.children[0] == &child1);

    // 2. Test appending another child
    ramfs_node_t child2;
    memset(&child2, 0, sizeof(child2));
    child2.vfs_node.flags = 1; // File

    assert(ramfs_add_child(&parent, &child2) == 0);
    assert(parent.dir.child_count == 2);
    assert(parent.dir.children[0] == &child1);
    assert(parent.dir.children[1] == &child2);

    kfree(parent.dir.children);
}

void test_ramfs_add_child_growth() {
    // Test growing the capacity
    ramfs_node_t parent;
    memset(&parent, 0, sizeof(parent));
    parent.vfs_node.flags = 2; // Directory

    // Set up small capacity to trigger growth
    parent.dir.child_capacity = 2;
    parent.dir.children = (ramfs_node_t **)kmalloc(sizeof(ramfs_node_t *) * parent.dir.child_capacity);
    assert(parent.dir.children != NULL);
    parent.dir.child_count = 0;

    ramfs_node_t children[3];
    for (int i = 0; i < 3; i++) {
        memset(&children[i], 0, sizeof(ramfs_node_t));
        children[i].vfs_node.flags = 1; // File
        assert(ramfs_add_child(&parent, &children[i]) == 0);
    }

    assert(parent.dir.child_count == 3);
    assert(parent.dir.child_capacity == 4); // Capacity should double from 2 to 4
    for (int i = 0; i < 3; i++) {
        assert(parent.dir.children[i] == &children[i]);
    }

    kfree(parent.dir.children);
}

int main() {
    printf("Running ramfs_add_child tests...\n");
    test_ramfs_add_child_error_paths();
    test_ramfs_add_child_success();
    test_ramfs_add_child_growth();
    printf("ramfs_add_child tests passed.\n");
    return 0;
}
