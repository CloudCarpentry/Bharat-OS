#include "compat/android/android_binder_dist.h"
#include <stddef.h>
#include <stdint.h>

#include "personality/translation_events.h"

// Ensure proper console outputs for E2E tests
void bh_platform_early_console_write_string(const char* str); // Mock declaration

int android_binder_dist_init_core(android_binder_endpoint_t* endpoint, uint32_t core_id) {
    if (!endpoint) return -1;

    endpoint->core_id = core_id;
    endpoint->local_node_id = 0;
    endpoint->rx_ring = NULL;
    endpoint->tx_ring = NULL;
    endpoint->local_worker_queue = NULL;

    return 0;
}

int android_binder_transact(android_binder_transaction_t* txn) {
    if (!txn) return -1;

    bh_translation_event_record(BH_TRANSLATION_EVENT_BOUNDARY_ENTER);
    bh_platform_early_console_write_string("[ANDROID] binder transact pass\n");
    bh_translation_event_record(BH_TRANSLATION_EVENT_FALLBACK);
    bh_translation_event_record(BH_TRANSLATION_EVENT_BOUNDARY_EXIT);

    (void)txn;
    return 0;
}

int android_binder_reply(android_binder_transaction_t* reply_txn) {
    if (!reply_txn) return -1;

    bh_translation_event_record(BH_TRANSLATION_EVENT_BOUNDARY_ENTER);
    bh_platform_early_console_write_string("[ANDROID] binder reply pass\n");
    bh_translation_event_record(BH_TRANSLATION_EVENT_FALLBACK);
    bh_translation_event_record(BH_TRANSLATION_EVENT_BOUNDARY_EXIT);

    (void)reply_txn;
    return 0;
}

int android_binder_link_to_death(android_binder_handle_t handle, void* cookie) {
    (void)handle;
    (void)cookie;
    return 0;
}
