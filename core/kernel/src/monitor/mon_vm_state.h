#ifndef BHARAT_MON_VM_STATE_H
#define BHARAT_MON_VM_STATE_H

#include "../../include/monitor/mon_vm_proto.h"
#include "../../include/spinlock.h"
#include <stdint.h>
#include <stdbool.h>

#define MON_VM_INBOX_SLOTS 64
#define MON_VM_MAX_CPUS 64
#define MON_VM_REPLAY_CACHE_SIZE 64
#define MAX_PENDING_TXNS 64

typedef enum {
    MON_VM_STATUS_SUCCESS = 0,
    MON_VM_STATUS_TIMEOUT = -1,
    MON_VM_STATUS_NACK = -2,
    MON_VM_STATUS_CHANNEL_ERROR = -3,
    MON_VM_STATUS_MALFORMED = -4,
    MON_VM_STATUS_UNSUPPORTED = -5,
    MON_VM_STATUS_BUSY = -6
} mon_vm_status_t;

// Transaction phases
typedef enum {
    MON_VM_TX_PHASE_PREPARED = 0,
    MON_VM_TX_PHASE_SENT,
    MON_VM_TX_PHASE_COMMITTED,
    MON_VM_TX_PHASE_ABORTED
} mon_vm_tx_phase_t;

// Bounded pending transaction record (fully owned by the origin core)
typedef struct {
    mon_vm_tx_handle_t handle;
    bool in_use;
    uint32_t op;                  // mon_vm_msg_type_t
    uint64_t space_id;
    uint64_t expected_generation;
    uint64_t proposed_generation;

    uint64_t target_mask;         // Cores requested
    uint64_t ack_mask;            // Cores that responded
    uint64_t success_mask;        // Cores that succeeded
    uint64_t nack_mask;           // Cores that failed

    uint32_t attempt_count;
    uint64_t deadline_ticks;      // Monotonic tick deadline
    int32_t final_status;
    mon_vm_tx_phase_t phase;
} mon_vm_pending_txn_t;

// Inbox slot structure
typedef struct {
    volatile uint32_t state;      // 0 = free, 1 = reserved, 2 = published
    uint32_t generation;
    mon_vm_msg_t msg;
} mon_vm_inbox_slot_t;

// Receiver-side Replay Cache Entry
typedef struct {
    uint16_t origin_core;
    uint16_t slot;
    uint32_t generation;
    uint32_t op;
    int32_t status;
    bool active;
} mon_vm_replay_entry_t;

// Per-core Monitor State (strictly local to each CPU)
typedef struct {
    spinlock_t lock;
    bool initialized;

    // Transactions initiated by this core
    uint32_t next_txn_generation;
    mon_vm_pending_txn_t pending_txns[MAX_PENDING_TXNS];

    // Inbox of this core (receives messages from other cores)
    mon_vm_inbox_slot_t inbox[MON_VM_INBOX_SLOTS];

    // Replay cache for idempotence
    mon_vm_replay_entry_t replay_cache[MON_VM_REPLAY_CACHE_SIZE];
    uint32_t replay_cache_index;

    // Local realization registry: active space IDs realized on this core
    uint64_t realized_spaces_mask;

    // Observability / Telemetry
    uint64_t stat_requests_sent;
    uint64_t stat_requests_received;
    uint64_t stat_acks_sent;
    uint64_t stat_acks_received;
    uint64_t stat_timeouts;
    uint64_t stat_failures;
    uint64_t stat_replays;
} mon_vm_core_state_t;

// Backward-compatibility shim struct
typedef struct {
    bool initialized;
    uint64_t next_txn_id;
    uint64_t stat_requests_sent;
    uint64_t stat_requests_received;
    uint64_t stat_acks_sent;
    uint64_t stat_acks_received;
    uint64_t stat_timeouts;
    uint64_t stat_failures;
} mon_vm_state_t;

// Per-core global monitor states array
extern mon_vm_core_state_t g_mon_vm_core_states[MON_VM_MAX_CPUS];

// Helper to access current CPU's state safely
mon_vm_core_state_t* mon_vm_get_local_state(void);

// Function Prototypes for across-files access
int mon_vm_reserve_inbox_slot(uint32_t dst_core, uint32_t *out_slot_idx, uint32_t *out_slot_gen);
void mon_vm_publish_inbox_slot(uint32_t dst_core, uint32_t slot_idx, const mon_vm_msg_t *msg);
int mon_vm_send_txn_async(mon_vm_pending_txn_t *txn, const mon_vm_msg_t *msg_template);
int mon_vm_check_replay(uint16_t origin_core, uint16_t slot, uint32_t generation, uint32_t op, int32_t *out_status);
void mon_vm_record_replay(uint16_t origin_core, uint16_t slot, uint32_t generation, uint32_t op, int32_t status);
void mon_vm_poll_inbox(void);

// Backwards compatibility shim for global tests
extern mon_vm_state_t g_mon_vm_state;

#endif // BHARAT_MON_VM_STATE_H
