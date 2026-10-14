#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "compat/android/android_binder_dist.h"
#include "compat/android/android_personality.h"
#include "compat/android/android_security.h"

/* Mock for bh_translation_event_record in host test environments */
void bh_translation_event_record(int event) {
    (void)event;
}

static void test_binder_endpoint_and_personality(void) {
    printf("  Testing Binder endpoint and personality initialization...\n");

    android_binder_endpoint_t ep0, ep1;
    assert(android_binder_dist_init_core(&ep0, 0) == 0);
    assert(ep0.core_id == 0);
    assert(ep0.rx_ring != NULL);

    assert(android_binder_dist_init_core(&ep1, 1) == 0);
    assert(ep1.core_id == 1);
    assert(ep1.rx_ring != NULL);

    /* Null endpoint must fail closed */
    assert(android_binder_dist_init_core(NULL, 0) == -1);

    android_personality_t p;
    memset(&p, 0, sizeof(p));
    assert(android_personality_register(&p) == 0);
    assert(p.base.id == 3); /* PERS_TYPE_ANDROID is 3 */
    assert(strcmp(p.base.name, "Android") == 0);
    assert(p.base.init != NULL);
    assert(p.base.init(&p.base) == 0);
    assert(p.max_binder_nodes_per_core == 1024);
    assert(p.service_manager_root.id == 0);
    assert(p.service_manager_root.generation == 1);
}

static void test_binder_node_and_object_lifecycle(void) {
    printf("  Testing Binder service node lifecycle and object lookup...\n");

    /* ServiceManager root object lookup (ID 0) */
    android_logical_obj_t root_obj;
    assert(android_obj_lookup(0, &root_obj) == 0);
    assert(root_obj.id == 0);
    assert(root_obj.home_core == 0);
    assert(root_obj.generation == 1);

    /* Lookup non-existent object must fail closed */
    assert(android_obj_lookup(999, &root_obj) < 0);

    /* Register dynamic object */
    android_logical_obj_t dyn_obj = {
        .id = 42,
        .home_core = 1,
        .generation = 5,
        .backing_cap = {0}
    };
    assert(android_obj_register(&dyn_obj) == 0);

    android_logical_obj_t queried;
    assert(android_obj_lookup(42, &queried) == 0);
    assert(queried.id == 42);
    assert(queried.home_core == 1);
    assert(queried.generation == 5);

    /* Unregister dynamic object */
    assert(android_obj_unregister(42) == 0);
    assert(android_obj_lookup(42, &queried) < 0);

    /* Unregister root (0) must be rejected */
    assert(android_obj_unregister(0) < 0);
}

static void test_binder_transactions_and_death(void) {
    printf("  Testing Binder transactions, error states, and death notifications...\n");

    /* Register a test service node */
    android_binder_handle_t handle;
    assert(android_binder_node_register(100, 0, 10, ANDROID_SEC_SID_SYSTEM_SERVER, &handle) == 0);
    assert(handle.service_id == 100);
    assert(handle.generation == 1);

    /* Link to death */
    void* cookie = (void*)0xDEADBEEF;
    assert(android_binder_link_to_death(handle, cookie) == 0);

    /* Linking death to non-existent handle must fail closed */
    android_binder_handle_t invalid_handle = { .service_id = 888, .generation = 1 };
    assert(android_binder_link_to_death(invalid_handle, cookie) < 0);

    /* Linking death with NULL cookie must fail closed */
    assert(android_binder_link_to_death(handle, NULL) < 0);

    /* Default transaction to ServiceManager (service_id = 0) */
    android_binder_transaction_t txn;
    memset(&txn, 0, sizeof(txn));
    assert(android_binder_transact(&txn) == 0);
    assert(android_binder_reply(&txn) == 0);

    /* Valid transaction to node 100 */
    android_binder_transaction_t valid_txn = {
        .type = BINDER_TRANS_SYNC,
        .target = handle,
        .sender_pid = 20,
        .sender_sid = ANDROID_SEC_SID_APP,
        .code = ANDROID_BINDER_ACTION_CALL,
        .payload_ptr = NULL,
        .payload_size = 0
    };
    assert(android_binder_transact(&valid_txn) == 0);

    /* Asynchronous one-way transaction */
    valid_txn.type = BINDER_TRANS_ASYNC;
    assert(android_binder_transact(&valid_txn) == 0);

    /* Negative: NULL transaction must fail */
    assert(android_binder_transact(NULL) == -1);
    assert(android_binder_reply(NULL) == -1);

    /* Negative: Stale generation must fail closed */
    android_binder_transaction_t stale_txn = valid_txn;
    stale_txn.target.generation = 99;
    assert(android_binder_transact(&stale_txn) < 0);

    /* Negative: Non-existent node must fail closed */
    android_binder_transaction_t nonexist_txn = valid_txn;
    nonexist_txn.target.service_id = 777;
    assert(android_binder_transact(&nonexist_txn) < 0);

    /* Negative: Corrupted payload pointer with non-zero size */
    android_binder_transaction_t corrupt_txn = valid_txn;
    corrupt_txn.payload_size = 512;
    corrupt_txn.payload_ptr = NULL;
    assert(android_binder_transact(&corrupt_txn) < 0);

    /* Destroy node 100 (triggers death notification) */
    assert(android_binder_node_destroy(100) == 0);

    /* Transactions to destroyed node must now fail closed */
    assert(android_binder_transact(&valid_txn) < 0);
}

int main(void) {
    printf("Running test_android_binder (Full Distributed Binder Suite)...\n");

    test_binder_endpoint_and_personality();
    test_binder_node_and_object_lifecycle();
    test_binder_transactions_and_death();

    printf("test_android_binder passed!\n");
    return 0;
}
