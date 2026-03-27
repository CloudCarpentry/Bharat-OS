#ifndef BHARAT_MON_VM_STATE_H
#define BHARAT_MON_VM_STATE_H

#include "../../include/monitor/mon_vm_proto.h"
#include "../../include/spinlock.h"
#include <stdint.h>
#include <stdbool.h>

#define spinlock_acquire(lock) spin_lock(lock)
#define spinlock_release(lock) spin_unlock(lock)

#define MAX_PENDING_TXNS 64

typedef enum {
    MON_VM_STATUS_SUCCESS = 0,
    MON_VM_STATUS_TIMEOUT = -1,
    MON_VM_STATUS_NACK = -2,
    MON_VM_STATUS_CHANNEL_ERROR = -3,
    MON_VM_STATUS_MALFORMED = -4,
    MON_VM_STATUS_UNSUPPORTED = -5
} mon_vm_status_t;

typedef struct {
    uint64_t txn_id;
    bool in_use;
    uint64_t expected_acks_mask;
    uint64_t received_acks_mask;
    int32_t final_status;
    uint32_t start_ticks;
} mon_vm_pending_txn_t;

typedef struct {
    spinlock_t lock;
    bool initialized;
    uint64_t next_txn_id;
    mon_vm_pending_txn_t pending_txns[MAX_PENDING_TXNS];

    // Observability / Telemetry
    uint64_t stat_requests_sent;
    uint64_t stat_requests_received;
    uint64_t stat_acks_sent;
    uint64_t stat_acks_received;
    uint64_t stat_timeouts;
    uint64_t stat_failures;
} mon_vm_state_t;

extern mon_vm_state_t g_mon_vm_state;

#endif // BHARAT_MON_VM_STATE_H
