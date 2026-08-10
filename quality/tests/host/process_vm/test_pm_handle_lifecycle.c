#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <handle_table.h>

enum {
    TEST_PROCESS_OBJECT_TYPE = 1,
    TEST_PROCESS_RIGHTS = 7,
    TEST_REPLACEMENT_PROCESS_RIGHTS = 5,
};

void test_handle_table_lifecycle(void) {
    bh_user_handle_table_t table;
    bh_user_handle_slot_t slots[4];

    int res = bh_user_handle_table_init(&table, slots, 4);
    assert(res == BH_HANDLE_TABLE_SUCCESS);
    assert(table.count == 0);
    assert(table.capacity == 4);

    int process_object = 42;
    int replacement_process_object = 100;

    bh_handle_t handle1 = 0;
    res = bh_user_handle_alloc(&table, &process_object, TEST_PROCESS_OBJECT_TYPE,
                               TEST_PROCESS_RIGHTS, &handle1);
    assert(res == BH_HANDLE_TABLE_SUCCESS);
    assert(table.count == 1);
    assert(bh_user_handle_index(handle1) == 0);
    assert(bh_user_handle_generation(handle1) == 1); // generation starts at 1, never 0

    void *ret_obj = NULL;
    uint64_t ret_rights = 0;
    res = bh_user_handle_lookup(&table, handle1, TEST_PROCESS_OBJECT_TYPE,
                                &ret_obj, &ret_rights);
    assert(res == BH_HANDLE_TABLE_SUCCESS);
    assert(*(int *)ret_obj == 42);
    assert(ret_rights == TEST_PROCESS_RIGHTS);

    // Revoke
    res = bh_user_handle_revoke(&table, handle1);
    assert(res == BH_HANDLE_TABLE_SUCCESS);
    assert(table.count == 0);

    // Stale lookup should fail
    res = bh_user_handle_lookup(&table, handle1, TEST_PROCESS_OBJECT_TYPE,
                                &ret_obj, NULL);
    assert(res == BH_HANDLE_TABLE_ERR_NOT_FOUND);

    // Allocate again on same slot - should increment generation to 2
    bh_handle_t handle2 = 0;
    res = bh_user_handle_alloc(&table, &replacement_process_object,
                               TEST_PROCESS_OBJECT_TYPE,
                               TEST_REPLACEMENT_PROCESS_RIGHTS, &handle2);
    assert(res == BH_HANDLE_TABLE_SUCCESS);
    assert(bh_user_handle_index(handle2) == 0);
    assert(bh_user_handle_generation(handle2) == 2);

    // Lookup on stale handle1 should still fail
    res = bh_user_handle_lookup(&table, handle1, TEST_PROCESS_OBJECT_TYPE,
                                &ret_obj, NULL);
    assert(res == BH_HANDLE_TABLE_ERR_NOT_FOUND);

    // Lookup on active handle2 should succeed
    res = bh_user_handle_lookup(&table, handle2, TEST_PROCESS_OBJECT_TYPE,
                                &ret_obj, NULL);
    assert(res == BH_HANDLE_TABLE_SUCCESS);
    assert(*(int *)ret_obj == 100);

    printf("test_handle_table_lifecycle passed!\n");
}

int main(void) {
    test_handle_table_lifecycle();
    return 0;
}
