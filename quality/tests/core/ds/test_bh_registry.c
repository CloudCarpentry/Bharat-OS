#include <bharat/kernel/ds/bh_registry.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

#define TEST_CAPACITY 10

void test_registry_basic() {
    bh_registry_t reg;
    bh_registry_entry_t storage[TEST_CAPACITY];

    assert(bh_registry_init(&reg, storage, TEST_CAPACITY) == K_OK);
    assert(bh_registry_count(&reg) == 0);

    int obj1 = 1;
    int obj2 = 2;

    assert(bh_registry_register(&reg, 100, "comp1", &obj1, 0) == K_OK);
    assert(bh_registry_register(&reg, 200, "comp2", &obj2, 0) == K_OK);
    assert(bh_registry_count(&reg) == 2);

    assert(bh_registry_lookup_id(&reg, 100) == &obj1);
    assert(bh_registry_lookup_name(&reg, "comp2") == &obj2);
    assert(bh_registry_lookup_id(&reg, 300) == NULL);
    assert(bh_registry_lookup_name(&reg, "comp3") == NULL);

    printf("test_registry_basic passed\n");
}

void test_registry_duplicates() {
    bh_registry_t reg;
    bh_registry_entry_t storage[TEST_CAPACITY];
    bh_registry_init(&reg, storage, TEST_CAPACITY);

    int obj1 = 1;
    assert(bh_registry_register(&reg, 100, "comp1", &obj1, 0) == K_OK);

    /* Duplicate ID */
    assert(bh_registry_register(&reg, 100, "comp2", &obj1, 0) == K_ERR_ALREADY_EXISTS);

    /* Duplicate Name */
    assert(bh_registry_register(&reg, 101, "comp1", &obj1, 0) == K_ERR_ALREADY_EXISTS);

    printf("test_registry_duplicates passed\n");
}

void test_registry_freeze() {
    bh_registry_t reg;
    bh_registry_entry_t storage[TEST_CAPACITY];
    bh_registry_init(&reg, storage, TEST_CAPACITY);

    int obj1 = 1;
    assert(bh_registry_register(&reg, 100, "comp1", &obj1, 0) == K_OK);

    assert(bh_registry_freeze(&reg) == K_OK);

    assert(bh_registry_register(&reg, 200, "comp2", &obj1, 0) == K_ERR_BAD_STATE);

    /* Lookups should still work */
    assert(bh_registry_lookup_id(&reg, 100) == &obj1);

    printf("test_registry_freeze passed\n");
}

int main() {
    test_registry_basic();
    test_registry_duplicates();
    test_registry_freeze();
    return 0;
}
