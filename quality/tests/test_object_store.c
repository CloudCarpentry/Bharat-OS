#include <stdio.h>
#include <string.h>

#include "fs/object_store.h"
#include "../kernel/staging/formal/formal_verif.h"

// Reset function provided by kernel/src/fs/object_store.c when compiled with TESTING
extern void object_store_test_reset_state(void);

int main() {
    int fails = 0;

    object_store_t my_store;
    memset(&my_store, 0, sizeof(my_store));
    strcpy(my_store.uri, "/blob/remote/bucket");

    capability_t test_cap;
    memset(&test_cap, 0, sizeof(test_cap));
    test_cap.rights_mask = 1 | 2; // Old CAP_RIGHT_READ | CAP_RIGHT_WRITE

    // Set up the store.
    object_store_test_reset_state();
    if (object_store_register(&my_store, &test_cap) != 0) {
        printf("FAILED: Store registration failed\n");
        fails++;
    }

    object_store_t* out_store = NULL;

    // Test 1: NULL cap should fail
    if (object_store_lookup("/blob/remote/bucket", &out_store, NULL) == 0) {
        printf("FAILED: Expected failure with NULL cap\n");
        fails++;
    }

    // Test 2: cap without CAP_RIGHT_READ should fail
    capability_t no_read_cap;
    memset(&no_read_cap, 0, sizeof(no_read_cap));
    no_read_cap.rights_mask = 2; // Old CAP_RIGHT_WRITE

    if (object_store_lookup("/blob/remote/bucket", &out_store, &no_read_cap) == 0) {
        printf("FAILED: Expected failure without CAP_RIGHT_READ\n");
        fails++;
    }

    // Test 3: cap with CAP_RIGHT_READ should succeed
    capability_t read_cap;
    memset(&read_cap, 0, sizeof(read_cap));
    read_cap.rights_mask = 1; // Old CAP_RIGHT_READ

    out_store = NULL;
    if (object_store_lookup("/blob/remote/bucket", &out_store, &read_cap) != 0 || out_store != &my_store) {
        printf("FAILED: Expected success with CAP_RIGHT_READ\n");
        fails++;
    }

    // Test 4: cap with CAP_RIGHT_READ | CAP_RIGHT_WRITE should succeed
    capability_t read_write_cap;
    memset(&read_write_cap, 0, sizeof(read_write_cap));
    read_write_cap.rights_mask = 1 | 2; // Old CAP_RIGHT_READ | CAP_RIGHT_WRITE

    out_store = NULL;
    if (object_store_lookup("/blob/remote/bucket", &out_store, &read_write_cap) != 0 || out_store != &my_store) {
        printf("FAILED: Expected success with CAP_RIGHT_READ | CAP_RIGHT_WRITE\n");
        fails++;
    }

    // Test 5: non-matching URI with correct cap should fail
    if (object_store_lookup("/blob/nonexistent", &out_store, &read_cap) == 0) {
        printf("FAILED: Expected failure with nonexistent URI\n");
        fails++;
    }

    if (fails > 0) {
        printf("TESTS FAILED: %d failures\n", fails);
        return 1;
    }

    printf("All object store capability tests passed.\n");
    return 0;
}
