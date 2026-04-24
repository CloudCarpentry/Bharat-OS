#ifndef BHARAT_ANDROID_BINDER_DIST_H
#define BHARAT_ANDROID_BINDER_DIST_H

#include "android_personality.h"

/*
 * Phase 2: Distributed Binder Skeleton
 *
 * Defines the Binder object ID / handle format, local binder endpoint abstraction,
 * cross-core transport, transaction packet types, reply contracts, and priority
 * propagation metadata.
 *
 * In Bharat-OS, Binder is NOT a monolithic driver. It is a distributed fabric
 * connecting per-core endpoint shards.
 */

// Handles are not raw pointers; they encode routing metadata
typedef struct {
    uint32_t service_id;
    uint32_t home_core;
    uint32_t generation;
    uint32_t rights_flags;
} android_binder_handle_t;

// Transaction Priority Metadata
typedef struct {
    uint32_t caller_priority;
    uint32_t caller_scheduling_class;
    int has_priority_inheritance;
} android_binder_priority_t;

// Transaction Types
typedef enum {
    BINDER_TRANS_SYNC = 1,
    BINDER_TRANS_ASYNC = 2,
    BINDER_REPLY_SYNC = 3,
    BINDER_NODE_DEATH_NOTIFY = 4
} android_binder_msg_type_t;

// Transaction Packet format (Distributed)
typedef struct {
    android_binder_msg_type_t type;
    android_binder_handle_t target;
    android_binder_priority_t priority;

    uint32_t code;
    uint32_t flags;

    // SELinux and Process Identity
    uint32_t sender_euid;
    uint32_t sender_pid;
    uint32_t sender_sid;

    // Payload (mapped via shared capability regions, not copied if large)
    void* payload_ptr;
    uint32_t payload_size;
} android_binder_transaction_t;

// Local Endpoint Abstraction (Per-Core Worker Pool)
typedef struct {
    uint32_t local_node_id;
    uint32_t core_id;

    // Fast-path capability queues for incoming/outgoing transactions
    void* rx_ring;
    void* tx_ring;

    // Wait queues for local worker threads sleeping on /dev/binder equivalent
    void* local_worker_queue;
} android_binder_endpoint_t;

/**
 * @brief Initialize the distributed Binder subsystem on the current core.
 */
int android_binder_dist_init_core(android_binder_endpoint_t* endpoint, uint32_t core_id);

/**
 * @brief Send a Binder transaction across the distributed fabric.
 * Translates the handle to a home_core and uses Inter-Core Communication (ICC).
 */
int android_binder_transact(android_binder_transaction_t* txn);

/**
 * @brief Synchronous Reply Path.
 * Returns the result of a synchronous transaction back to the waiting caller core.
 */
int android_binder_reply(android_binder_transaction_t* reply_txn);

/**
 * @brief Register a death notification for a remote Binder handle.
 */
int android_binder_link_to_death(android_binder_handle_t handle, void* cookie);

#endif // BHARAT_ANDROID_BINDER_DIST_H
