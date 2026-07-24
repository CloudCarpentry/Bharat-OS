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

// vfs_test_reset_state is used in some tests but not strictly needed for ramfs.c itself
// but we include ramfs.c which might have some dependencies if we're not careful.

// Now include the source file to test the static function
#include "../../../kernel/src/fs/ramfs.c"

void test_ramfs_streq() {
    // Normal cases
    assert(ramfs_streq("hello", "hello") == 1);
    assert(ramfs_streq("hello", "world") == 0);
    assert(ramfs_streq("", "") == 1);
    assert(ramfs_streq("a", "b") == 0);

    // Prefix cases
    assert(ramfs_streq("hell", "hello") == 0);
    assert(ramfs_streq("hello", "hell") == 0);

    // Error/Edge cases (NULL inputs)
    assert(ramfs_streq(NULL, "hello") == 0);
    assert(ramfs_streq("hello", NULL) == 0);
    assert(ramfs_streq(NULL, NULL) == 0);
}

int main() {
    printf("Running ramfs_streq tests...\n");
    test_ramfs_streq();
    printf("ramfs_streq tests passed.\n");
    return 0;
}
