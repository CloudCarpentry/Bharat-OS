#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <handle_table.h>

void test_vm_handle_table_lifecycle(void) {
    bh_user_handle_table_t table;
    bh_user_handle_slot_t slots[8];

    int res = bh_user_handle_table_init(&table, slots, 8);
    assert(res == BH_HANDLE_TABLE_SUCCESS);
    assert(table.count == 0);

    int dummy_space = 11;
    bh_handle_t handle = 0;
    res = bh_user_handle_alloc(&table, &dummy_space, 2002, 1, &handle);
    assert(res == BH_HANDLE_TABLE_SUCCESS);
    assert(bh_user_handle_index(handle) == 0);
    assert(bh_user_handle_generation(handle) == 1);

    void *ret_obj = NULL;
    res = bh_user_handle_lookup(&table, handle, 2002, &ret_obj, NULL);
    assert(res == BH_HANDLE_TABLE_SUCCESS);
    assert(*(int *)ret_obj == 11);

    // Double revoke check
    res = bh_user_handle_revoke(&table, handle);
    assert(res == BH_HANDLE_TABLE_SUCCESS);
    res = bh_user_handle_revoke(&table, handle);
    assert(res == BH_HANDLE_TABLE_ERR_NOT_FOUND);

    printf("test_vm_handle_table_lifecycle passed!\n");
}

int main(void) {
    test_vm_handle_table_lifecycle();
    return 0;
}
