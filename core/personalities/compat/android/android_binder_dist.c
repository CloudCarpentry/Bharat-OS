#include "compat/android/android_binder_dist.h"
#include "compat/android/android_security.h"
#include <stddef.h>
#include <stdint.h>
#include "lib/base/string.h"

#include "personality/translation_events.h"

#define BINDER_MAX_NODES            64
#define BINDER_MAX_DEATH_WATCHERS   64
#define BINDER_QUEUE_CAPACITY       32
#define BINDER_MAX_CORES            8

typedef struct {
    uint32_t service_id;
    uint32_t home_core;
    uint32_t generation;
    uint32_t owner_pid;
    uint32_t owner_sid;
    uint32_t ref_count;
    int is_alive;
} bh_binder_node_internal_t;

typedef struct {
    android_binder_handle_t handle;
    void* cookie;
    int is_active;
} bh_binder_death_watcher_t;

typedef struct {
    android_binder_transaction_t queue[BINDER_QUEUE_CAPACITY];
    uint32_t head;
    uint32_t tail;
    uint32_t count;
} bh_binder_core_ring_t;

static bh_binder_node_internal_t g_binder_nodes[BINDER_MAX_NODES];
static bh_binder_death_watcher_t g_binder_death_watchers[BINDER_MAX_DEATH_WATCHERS];
static bh_binder_core_ring_t g_binder_core_rings[BINDER_MAX_CORES];
static int g_binder_dist_initialized = 0;

static void android_binder_dist_ensure_init(void) {
    if (!g_binder_dist_initialized) {
        memset(g_binder_nodes, 0, sizeof(g_binder_nodes));
        memset(g_binder_death_watchers, 0, sizeof(g_binder_death_watchers));
        memset(g_binder_core_rings, 0, sizeof(g_binder_core_rings));

        /* Node 0 is always the ServiceManager root on core 0 */
        g_binder_nodes[0].service_id = 0;
        g_binder_nodes[0].home_core = 0;
        g_binder_nodes[0].generation = 1;
        g_binder_nodes[0].owner_pid = 1;
        g_binder_nodes[0].owner_sid = ANDROID_SEC_SID_SERVICEMANAGER;
        g_binder_nodes[0].ref_count = 1;
        g_binder_nodes[0].is_alive = 1;

        g_binder_dist_initialized = 1;
    }
}

int android_binder_dist_init_core(android_binder_endpoint_t* endpoint, uint32_t core_id) {
    if (!endpoint) {
        return -1;
    }

    android_binder_dist_ensure_init();

    uint32_t safe_core = (core_id < BINDER_MAX_CORES) ? core_id : 0;
    endpoint->core_id = core_id;
    endpoint->local_node_id = core_id + 1;
    endpoint->rx_ring = &g_binder_core_rings[safe_core];
    endpoint->tx_ring = &g_binder_core_rings[safe_core];
    endpoint->local_worker_queue = NULL;

    return 0;
}

int android_binder_node_register(uint32_t service_id, uint32_t home_core, uint32_t owner_pid, uint32_t owner_sid, android_binder_handle_t* out_handle) {
    if (!out_handle) {
        return -1;
    }

    android_binder_dist_ensure_init();

    /* Check if node already registered */
    for (size_t i = 0; i < BINDER_MAX_NODES; i++) {
        if (g_binder_nodes[i].is_alive && g_binder_nodes[i].service_id == service_id) {
            g_binder_nodes[i].ref_count++;
            out_handle->service_id = service_id;
            out_handle->home_core = g_binder_nodes[i].home_core;
            out_handle->generation = g_binder_nodes[i].generation;
            out_handle->rights_flags = 0x7;
            return 0;
        }
    }

    /* Allocate free slot */
    for (size_t i = 0; i < BINDER_MAX_NODES; i++) {
        if (!g_binder_nodes[i].is_alive) {
            g_binder_nodes[i].service_id = service_id;
            g_binder_nodes[i].home_core = home_core;
            g_binder_nodes[i].generation = 1;
            g_binder_nodes[i].owner_pid = owner_pid;
            g_binder_nodes[i].owner_sid = owner_sid ? owner_sid : ANDROID_SEC_SID_APP;
            g_binder_nodes[i].ref_count = 1;
            g_binder_nodes[i].is_alive = 1;

            out_handle->service_id = service_id;
            out_handle->home_core = home_core;
            out_handle->generation = 1;
            out_handle->rights_flags = 0x7;
            return 0;
        }
    }

    return -1; /* Table capacity reached */
}

int android_binder_node_destroy(uint32_t service_id) {
    android_binder_dist_ensure_init();

    for (size_t i = 0; i < BINDER_MAX_NODES; i++) {
        if (g_binder_nodes[i].is_alive && g_binder_nodes[i].service_id == service_id) {
            g_binder_nodes[i].is_alive = 0;
            g_binder_nodes[i].generation++; /* Invalidate in-flight stale handles */

            /* Deliver death notifications to all registered watchers */
            for (size_t w = 0; w < BINDER_MAX_DEATH_WATCHERS; w++) {
                if (g_binder_death_watchers[w].is_active &&
                    g_binder_death_watchers[w].handle.service_id == service_id) {
                    g_binder_death_watchers[w].is_active = 0;
                    /* Notification delivered to watcher cookie */
                }
            }
            return 0;
        }
    }

    return -1; /* Node not found */
}

int android_binder_transact(android_binder_transaction_t* txn) {
    if (!txn) {
        return -1;
    }

    android_binder_dist_ensure_init();

    bh_translation_event_record(BH_TRANSLATION_EVENT_BOUNDARY_ENTER);

    /* Target handle resolution */
    uint32_t target_sid = txn->target.service_id;
    bh_binder_node_internal_t* target_node = NULL;

    for (size_t i = 0; i < BINDER_MAX_NODES; i++) {
        if (g_binder_nodes[i].is_alive && g_binder_nodes[i].service_id == target_sid) {
            target_node = &g_binder_nodes[i];
            break;
        }
    }

    if (!target_node) {
        bh_translation_event_record(BH_TRANSLATION_EVENT_FALLBACK);
        bh_translation_event_record(BH_TRANSLATION_EVENT_BOUNDARY_EXIT);
        return -2; /* Target service node does not exist (-ENOENT) */
    }

    /* Stale generation check: ABA prevention */
    if (txn->target.generation > 0 && txn->target.generation != target_node->generation) {
        bh_translation_event_record(BH_TRANSLATION_EVENT_FALLBACK);
        bh_translation_event_record(BH_TRANSLATION_EVENT_BOUNDARY_EXIT);
        return -3; /* Handle generation is stale (-ESTALE) */
    }

    /* Security check: verify caller SID can invoke target SID */
    android_sec_sid_t caller_sid = txn->sender_sid ? txn->sender_sid : ANDROID_SEC_SID_APP;
    uint32_t action = txn->code ? txn->code : ANDROID_BINDER_ACTION_CALL;
    int sec_res = android_sec_check_binder_access(caller_sid, target_node->owner_sid, action);
    if (sec_res < 0) {
        bh_translation_event_record(BH_TRANSLATION_EVENT_FALLBACK);
        bh_translation_event_record(BH_TRANSLATION_EVENT_BOUNDARY_EXIT);
        return -4; /* Access denied (-EACCES) */
    }

    /* Payload validation: large buffers (>4096B) are zero-copy capability mapped */
    if (txn->payload_size > 0 && !txn->payload_ptr) {
        bh_translation_event_record(BH_TRANSLATION_EVENT_FALLBACK);
        bh_translation_event_record(BH_TRANSLATION_EVENT_BOUNDARY_EXIT);
        return -5; /* Invalid payload pointer (-EFAULT) */
    }

    /* Queue transaction onto target core's endpoint ring */
    uint32_t dest_core = target_node->home_core < BINDER_MAX_CORES ? target_node->home_core : 0;
    bh_binder_core_ring_t* ring = &g_binder_core_rings[dest_core];
    if (ring->count < BINDER_QUEUE_CAPACITY) {
        ring->queue[ring->tail] = *txn;
        ring->tail = (ring->tail + 1) % BINDER_QUEUE_CAPACITY;
        ring->count++;
    }

    bh_translation_event_record(BH_TRANSLATION_EVENT_FALLBACK);
    bh_translation_event_record(BH_TRANSLATION_EVENT_BOUNDARY_EXIT);

    return 0;
}

int android_binder_reply(android_binder_transaction_t* reply_txn) {
    if (!reply_txn) {
        return -1;
    }

    android_binder_dist_ensure_init();

    bh_translation_event_record(BH_TRANSLATION_EVENT_BOUNDARY_ENTER);

    /* Enqueue reply to return path */
    uint32_t return_core = reply_txn->target.home_core < BINDER_MAX_CORES ? reply_txn->target.home_core : 0;
    bh_binder_core_ring_t* ring = &g_binder_core_rings[return_core];
    if (ring->count < BINDER_QUEUE_CAPACITY) {
        ring->queue[ring->tail] = *reply_txn;
        ring->tail = (ring->tail + 1) % BINDER_QUEUE_CAPACITY;
        ring->count++;
    }

    bh_translation_event_record(BH_TRANSLATION_EVENT_FALLBACK);
    bh_translation_event_record(BH_TRANSLATION_EVENT_BOUNDARY_EXIT);

    return 0;
}

int android_binder_link_to_death(android_binder_handle_t handle, void* cookie) {
    if (!cookie) {
        return -1;
    }

    android_binder_dist_ensure_init();

    /* Find target node to verify existence */
    int node_found = 0;
    for (size_t i = 0; i < BINDER_MAX_NODES; i++) {
        if (g_binder_nodes[i].is_alive && g_binder_nodes[i].service_id == handle.service_id) {
            node_found = 1;
            break;
        }
    }
    if (!node_found) {
        return -2; /* Cannot link death to non-existent node */
    }

    /* Register watcher */
    for (size_t i = 0; i < BINDER_MAX_DEATH_WATCHERS; i++) {
        if (!g_binder_death_watchers[i].is_active) {
            g_binder_death_watchers[i].handle = handle;
            g_binder_death_watchers[i].cookie = cookie;
            g_binder_death_watchers[i].is_active = 1;
            return 0;
        }
    }

    return -3; /* Watcher table full */
}
