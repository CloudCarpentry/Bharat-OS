#include <bharat/kernel/ds/bh_handle_table.h>
#include <assert.h>
#include <stdio.h>
#include <stdint.h>

void test_handle_table_basic() {
    bh_handle_slot_t slots[8];
    bh_handle_table_t table;
    kstatus_t status = bh_handle_table_init(&table, slots, 8);
    assert(status == K_OK);

    int obj1 = 123;
    bh_handle_t h1;
    status = bh_handle_alloc(&table, &obj1, 1, 0xFULL, &h1);
    assert(status == K_OK);
    assert(bh_handle_validate(&table, h1));

    void *lookup_obj = NULL;
    uint64_t lookup_rights = 0;
    status = bh_handle_lookup(&table, h1, 1, &lookup_obj, &lookup_rights);
    assert(status == K_OK);
    assert(*(int *)lookup_obj == 123);
    assert(lookup_rights == 0xFULL);

    // Type mismatch
    status = bh_handle_lookup(&table, h1, 2, &lookup_obj, &lookup_rights);
    assert(status == K_ERR_CAP_WRONG_TYPE);

    // Revoke
    status = bh_handle_revoke(&table, h1);
    assert(status == K_OK);
    assert(!bh_handle_validate(&table, h1));
    assert(bh_handle_lookup(&table, h1, 1, &lookup_obj, NULL) == K_ERR_NOT_FOUND);

    // Re-allocation should use same slot but different generation
    bh_handle_t h2;
    status = bh_handle_alloc(&table, &obj1, 1, 0xFULL, &h2);
    assert(status == K_OK);
    assert(bh_handle_index(h1) == bh_handle_index(h2));
    assert(bh_handle_generation(h1) != bh_handle_generation(h2));
    assert(!bh_handle_validate(&table, h1));
    assert(bh_handle_validate(&table, h2));

    printf("test_handle_table_basic passed\n");
}

int main() {
    test_handle_table_basic();
    return 0;
}
