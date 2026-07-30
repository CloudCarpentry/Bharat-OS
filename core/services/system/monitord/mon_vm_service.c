#include "monitor/mon_vm_ops.h"
#include "mm/vm_space.h"
#include "urpc/urpc_bootstrap.h"
#include "hal/hal.h"
#include "hal/hal_timer.h"
#include "bharat/cpu_local.h"
#include "mon_vm_state.h"
#include <stddef.h>

#ifdef BHARAT_HOST_TEST
#include <string.h>
#else
void *memcpy(void *dest, const void *src, size_t n);
void *memset(void *s, int c, size_t n);
#endif

// Forward declarations of conversions
extern uint64_t mon_vm_ticks_to_ms(uint64_t ticks);
extern uint64_t mon_vm_ms_to_ticks(uint64_t ms);

static spinlock_t g_init_lock = {0};

int mon_vm_init(void) {
    uint32_t my_core = hal_cpu_get_id();
    if (my_core >= MON_VM_MAX_CPUS) return MON_VM_STATUS_CHANNEL_ERROR;

    mon_vm_core_state_t *state = &g_mon_vm_core_states[my_core];
    spin_lock(&g_init_lock);
    if (state->initialized) {
        spin_unlock(&g_init_lock);
        return 0; // Already initialized
    }

    spin_lock_init(&state->lock);
    state->next_txn_generation = 1;
    for (int i = 0; i < MAX_PENDING_TXNS; i++) {
        state->pending_txns[i].in_use = false;
    }
    for (int i = 0; i < MON_VM_INBOX_SLOTS; i++) {
        state->inbox[i].state = 0; // free
        state->inbox[i].generation = 0;
    }
    for (int i = 0; i < MON_VM_REPLAY_CACHE_SIZE; i++) {
        state->replay_cache[i].active = false;
    }
    state->replay_cache_index = 0;
    state->realized_spaces_mask = 0;

    state->stat_requests_sent = 0;
    state->stat_requests_received = 0;
    state->stat_acks_sent = 0;
    state->stat_acks_received = 0;
    state->stat_timeouts = 0;
    state->stat_failures = 0;
    state->stat_replays = 0;

    // Backward compatibility shim fields
    g_mon_vm_state.initialized = true;

    state->initialized = true;
    spin_unlock(&g_init_lock);
    return 0;
}

// Bounded atomic slot reservation for multi-slot per-core MPSC inbox
int mon_vm_reserve_inbox_slot(uint32_t dst_core, uint32_t *out_slot_idx, uint32_t *out_slot_gen) {
    if (dst_core >= MON_VM_MAX_CPUS) return MON_VM_STATUS_CHANNEL_ERROR;
    mon_vm_core_state_t *dst_state = &g_mon_vm_core_states[dst_core];
    if (!dst_state->initialized) return MON_VM_STATUS_CHANNEL_ERROR;

    // Iterate through slots to find a free one and reserve atomically using CAS (0 = FREE -> 1 = WRITING)
    for (uint32_t i = 0; i < MON_VM_INBOX_SLOTS; i++) {
        volatile uint32_t *slot_state = &dst_state->inbox[i].state;
        if (__sync_bool_compare_and_swap(slot_state, 0, 1)) {
            *out_slot_idx = i;
            *out_slot_gen = dst_state->inbox[i].generation;
            return MON_VM_STATUS_SUCCESS;
        }
    }
    return MON_VM_STATUS_BUSY; // Inbox full
}

// Publish message in the slot using atomic store with release semantics
void mon_vm_publish_inbox_slot(uint32_t dst_core, uint32_t slot_idx, const mon_vm_msg_t *msg) {
    mon_vm_core_state_t *dst_state = &g_mon_vm_core_states[dst_core];

    // Copy payload by-value first
    memcpy(&dst_state->inbox[slot_idx].msg, msg, sizeof(mon_vm_msg_t));

    // Store published (2 = READY) state with release semantics to ensure visibility of payload writes
    __atomic_store_n(&dst_state->inbox[slot_idx].state, 2, __ATOMIC_RELEASE);
}

// Allocate an ABA-safe transaction handle
static mon_vm_pending_txn_t* alloc_txn(mon_vm_core_state_t *state, mon_vm_tx_handle_t *out_handle) {
    spin_lock(&state->lock);
    for (int i = 0; i < MAX_PENDING_TXNS; i++) {
        if (!state->pending_txns[i].in_use) {
            state->pending_txns[i].in_use = true;
            state->pending_txns[i].handle.origin_core = hal_cpu_get_id();
            state->pending_txns[i].handle.slot = i;
            state->pending_txns[i].handle.generation = state->next_txn_generation++;
            state->pending_txns[i].target_mask = 0;
            state->pending_txns[i].ack_mask = 0;
            state->pending_txns[i].success_mask = 0;
            state->pending_txns[i].nack_mask = 0;
            state->pending_txns[i].attempt_count = 0;
            state->pending_txns[i].deadline_ticks = 0;
            state->pending_txns[i].final_status = MON_VM_STATUS_SUCCESS;
            state->pending_txns[i].phase = MON_VM_TX_PHASE_PREPARED;

            *out_handle = state->pending_txns[i].handle;
            spin_unlock(&state->lock);
            return &state->pending_txns[i];
        }
    }
    spin_unlock(&state->lock);
    return NULL;
}

static void free_txn(mon_vm_core_state_t *state, uint16_t slot) {
    spin_lock(&state->lock);
    if (slot < MAX_PENDING_TXNS) {
        state->pending_txns[slot].in_use = false;
    }
    spin_unlock(&state->lock);
}

// Doorbell token encoder
static uint64_t mon_vm_encode_doorbell(uint8_t op_class, uint8_t origin_core, uint16_t slot, uint32_t slot_gen) {
    return ((uint64_t)op_class << 56) |
           ((uint64_t)origin_core << 48) |
           ((uint64_t)slot << 32) |
           (uint64_t)slot_gen;
}

// Doorbell token decoder
void mon_vm_decode_doorbell(uint64_t token, uint8_t *out_class, uint8_t *out_origin, uint16_t *out_slot, uint32_t *out_slot_gen) {
    *out_class = (uint8_t)(token >> 56);
    *out_origin = (uint8_t)(token >> 48);
    *out_slot = (uint16_t)(token >> 32);
    *out_slot_gen = (uint32_t)token;
}

// Transmit transaction asynchronously and update pending record
int mon_vm_send_txn_async(mon_vm_pending_txn_t *txn, const mon_vm_msg_t *msg_template) {
    uint32_t my_core = hal_cpu_get_id();
    mon_vm_core_state_t *state = &g_mon_vm_core_states[my_core];

    txn->attempt_count++;
    txn->deadline_ticks = hal_timer_monotonic_ticks() + mon_vm_ms_to_ticks(10); // 10ms per attempt
    txn->phase = MON_VM_TX_PHASE_SENT;

    uint64_t missing_cores = txn->target_mask & ~txn->ack_mask & ~txn->nack_mask;

    for (uint32_t core = 0; core < MON_VM_MAX_CPUS; core++) {
        if (core == my_core) continue; // Exclude origin core from uRPC
        if (missing_cores & (1ULL << core)) {
            uint32_t slot_idx = 0;
            uint32_t slot_gen = 0;
            int res = mon_vm_reserve_inbox_slot(core, &slot_idx, &slot_gen);
            if (res != MON_VM_STATUS_SUCCESS) {
                // Inbox full: abort or return error to trigger retry later
                return MON_VM_STATUS_BUSY;
            }

            // Construct customized wire message
            mon_vm_msg_t wire_msg;
            memcpy(&wire_msg, msg_template, sizeof(mon_vm_msg_t));
            wire_msg.h.src_core = my_core;
            wire_msg.h.dst_core = core;
            wire_msg.h.tx_origin_core = txn->handle.origin_core;
            wire_msg.h.tx_slot = txn->handle.slot;
            wire_msg.h.tx_generation = txn->handle.generation;

            mon_vm_publish_inbox_slot(core, slot_idx, &wire_msg);

            // Construct and transmit doorbell via urpc
            uint64_t doorbell = mon_vm_encode_doorbell(0x99, my_core, slot_idx, slot_gen);
            urpc_bootstrap_send(core, doorbell);
        }
    }

    spin_lock(&state->lock);
    state->stat_requests_sent++;
    spin_unlock(&state->lock);

    return MON_VM_STATUS_SUCCESS;
}

// Wait for acks with monotonic timer retries and target-mask recovery
int mon_vm_wait_for_acks(mon_vm_tx_handle_t handle, const mon_vm_msg_t *msg_template) {
    uint32_t my_core = hal_cpu_get_id();
    mon_vm_core_state_t *state = &g_mon_vm_core_states[my_core];
    if (handle.slot >= MAX_PENDING_TXNS) return MON_VM_STATUS_MALFORMED;

    mon_vm_pending_txn_t *txn = &state->pending_txns[handle.slot];
    if (!txn->in_use || txn->handle.generation != handle.generation) {
        return MON_VM_STATUS_MALFORMED;
    }

    // Fast path: if no remote cores are target_mask, return success immediately
    if (txn->target_mask == 0) {
        txn->phase = MON_VM_TX_PHASE_COMMITTED;
        return MON_VM_STATUS_SUCCESS;
    }

    // Outer attempt loop (max 3 attempts)
    while (txn->attempt_count < 3) {
        // Run loop until deadline
        while (hal_timer_monotonic_ticks() < txn->deadline_ticks) {
            // Process any incoming ACKs/NACKs locally (by-value)
            hal_core_poll_event();

            spin_lock(&state->lock);
            uint64_t responses = txn->ack_mask | txn->nack_mask;
            bool done = (responses & txn->target_mask) == txn->target_mask;
            int32_t status = txn->final_status;
            spin_unlock(&state->lock);

            if (done) {
                if (status != MON_VM_STATUS_SUCCESS) {
                    txn->phase = MON_VM_TX_PHASE_ABORTED;
                    return status;
                }
                txn->phase = MON_VM_TX_PHASE_COMMITTED;
                return MON_VM_STATUS_SUCCESS;
            }
        }

        // Timeout hit for this attempt. Retry missing target cores only
        spin_lock(&state->lock);
        state->stat_timeouts++;
        spin_unlock(&state->lock);

        // Retransmit only to missing target cores
        int send_res = mon_vm_send_txn_async(txn, msg_template);
        if (send_res != MON_VM_STATUS_SUCCESS) {
            // If we fail to send (e.g. queue full), we still consume attempts/time
            txn->deadline_ticks = hal_timer_monotonic_ticks() + mon_vm_ms_to_ticks(10);
        }
    }

    // Fully timed out after 3 attempts
    txn->final_status = MON_VM_STATUS_TIMEOUT;
    txn->phase = MON_VM_TX_PHASE_ABORTED;
    return MON_VM_STATUS_TIMEOUT;
}

// Receiver replay cache management for idempotence
int mon_vm_check_replay(uint16_t origin_core, uint16_t slot, uint32_t generation, uint32_t op, int32_t *out_status) {
    uint32_t my_core = hal_cpu_get_id();
    mon_vm_core_state_t *state = &g_mon_vm_core_states[my_core];

    spin_lock(&state->lock);
    for (int i = 0; i < MON_VM_REPLAY_CACHE_SIZE; i++) {
        if (state->replay_cache[i].active &&
            state->replay_cache[i].origin_core == origin_core &&
            state->replay_cache[i].slot == slot &&
            state->replay_cache[i].generation == generation &&
            state->replay_cache[i].op == op) {

            *out_status = state->replay_cache[i].status;
            state->stat_replays++;
            spin_unlock(&state->lock);
            return 1; // Replayed duplicate request
        }
    }
    spin_unlock(&state->lock);
    return 0; // New request
}

void mon_vm_record_replay(uint16_t origin_core, uint16_t slot, uint32_t generation, uint32_t op, int32_t status) {
    uint32_t my_core = hal_cpu_get_id();
    mon_vm_core_state_t *state = &g_mon_vm_core_states[my_core];

    spin_lock(&state->lock);
    uint32_t idx = state->replay_cache_index;
    state->replay_cache[idx].origin_core = origin_core;
    state->replay_cache[idx].slot = slot;
    state->replay_cache[idx].generation = generation;
    state->replay_cache[idx].op = op;
    state->replay_cache[idx].status = status;
    state->replay_cache[idx].active = true;

    state->replay_cache_index = (idx + 1) % MON_VM_REPLAY_CACHE_SIZE;
    spin_unlock(&state->lock);
}

// ---------------------------------------------------------------------------
// Unified Mon VM Send Implementations
// ---------------------------------------------------------------------------
int mon_vm_send_map(vm_space_t *space, const vm_map_req_t *req, bool strict) {
    (void)strict; // Treat all active mutations as strict for security closure
    if (!space || !req) return MON_VM_STATUS_MALFORMED;

    uint32_t my_core = hal_cpu_get_id();
    mon_vm_core_state_t *state = &g_mon_vm_core_states[my_core];
    if (!state->initialized) return MON_VM_STATUS_CHANNEL_ERROR;

    mon_vm_tx_handle_t handle;
    mon_vm_pending_txn_t *txn = alloc_txn(state, &handle);
    if (!txn) return MON_VM_STATUS_CHANNEL_ERROR;

    // Define target mask: all currently active and realized cores, excluding sender
    txn->target_mask = (space->realized_cores | space->active_cores) & ~(1ULL << my_core);
    txn->space_id = space->space_id;
    txn->expected_generation = space->generation;
    txn->proposed_generation = space->generation + 1;
    txn->op = MON_VM_MAP;

    mon_vm_msg_t msg = {0};
    msg.map.h.version_major = MON_VM_PROTO_VERSION_MAJOR;
    msg.map.h.version_minor = MON_VM_PROTO_VERSION_MINOR;
    msg.map.h.type = MON_VM_MAP;
    msg.map.h.flags = MON_VM_F_STRICT_ACK;
    msg.map.h.space_id = space->space_id;
    msg.map.h.expected_generation = space->generation;
    msg.map.h.proposed_generation = space->generation + 1;
    msg.map.h.payload_len = sizeof(mon_vm_map_msg_t) - sizeof(mon_vm_hdr_t);

    msg.map.va_start = req->va;
    msg.map.pa_start = req->pa;
    msg.map.length = req->len;
    msg.map.prot = req->prot;
    msg.map.mem_type = req->mem_type;
    msg.map.map_flags = req->map_flags;

    int send_res = mon_vm_send_txn_async(txn, &msg);
    if (send_res != MON_VM_STATUS_SUCCESS) {
        free_txn(state, handle.slot);
        return send_res;
    }

    int wait_res = mon_vm_wait_for_acks(handle, &msg);
    free_txn(state, handle.slot);
    return wait_res;
}

int mon_vm_send_unmap(vm_space_t *space, uintptr_t va, size_t len, bool strict) {
    (void)strict; // Always strict
    if (!space) return MON_VM_STATUS_MALFORMED;

    uint32_t my_core = hal_cpu_get_id();
    mon_vm_core_state_t *state = &g_mon_vm_core_states[my_core];
    if (!state->initialized) return MON_VM_STATUS_CHANNEL_ERROR;

    mon_vm_tx_handle_t handle;
    mon_vm_pending_txn_t *txn = alloc_txn(state, &handle);
    if (!txn) return MON_VM_STATUS_CHANNEL_ERROR;

    // Revocation operation: must target realized_cores | active_mask, excluding sender
    txn->target_mask = (space->realized_cores | space->active_cores) & ~(1ULL << my_core);
    txn->space_id = space->space_id;
    txn->expected_generation = space->generation;
    txn->proposed_generation = space->generation + 1;
    txn->op = MON_VM_UNMAP;

    mon_vm_msg_t msg = {0};
    msg.unmap.h.version_major = MON_VM_PROTO_VERSION_MAJOR;
    msg.unmap.h.version_minor = MON_VM_PROTO_VERSION_MINOR;
    msg.unmap.h.type = MON_VM_UNMAP;
    msg.unmap.h.flags = MON_VM_F_STRICT_ACK;
    msg.unmap.h.space_id = space->space_id;
    msg.unmap.h.expected_generation = space->generation;
    msg.unmap.h.proposed_generation = space->generation + 1;
    msg.unmap.h.payload_len = sizeof(mon_vm_unmap_msg_t) - sizeof(mon_vm_hdr_t);

    msg.unmap.va_start = va;
    msg.unmap.length = len;

    int send_res = mon_vm_send_txn_async(txn, &msg);
    if (send_res != MON_VM_STATUS_SUCCESS) {
        free_txn(state, handle.slot);
        return send_res;
    }

    int wait_res = mon_vm_wait_for_acks(handle, &msg);
    free_txn(state, handle.slot);
    return wait_res;
}

int mon_vm_send_protect(vm_space_t *space, uintptr_t va, size_t len, uint64_t prot, uint64_t mem_type, bool strict) {
    (void)strict; // Always strict
    if (!space) return MON_VM_STATUS_MALFORMED;

    uint32_t my_core = hal_cpu_get_id();
    mon_vm_core_state_t *state = &g_mon_vm_core_states[my_core];
    if (!state->initialized) return MON_VM_STATUS_CHANNEL_ERROR;

    mon_vm_tx_handle_t handle;
    mon_vm_pending_txn_t *txn = alloc_txn(state, &handle);
    if (!txn) return MON_VM_STATUS_CHANNEL_ERROR;

    // Target active and realized core set
    txn->target_mask = (space->realized_cores | space->active_cores) & ~(1ULL << my_core);
    txn->space_id = space->space_id;
    txn->expected_generation = space->generation;
    txn->proposed_generation = space->generation + 1;
    txn->op = MON_VM_PROTECT;

    mon_vm_msg_t msg = {0};
    msg.protect.h.version_major = MON_VM_PROTO_VERSION_MAJOR;
    msg.protect.h.version_minor = MON_VM_PROTO_VERSION_MINOR;
    msg.protect.h.type = MON_VM_PROTECT;
    msg.protect.h.flags = MON_VM_F_STRICT_ACK;
    msg.protect.h.space_id = space->space_id;
    msg.protect.h.expected_generation = space->generation;
    msg.protect.h.proposed_generation = space->generation + 1;
    msg.protect.h.payload_len = sizeof(mon_vm_protect_msg_t) - sizeof(mon_vm_hdr_t);

    msg.protect.va_start = va;
    msg.protect.length = len;
    msg.protect.prot = prot;
    msg.protect.mem_type = mem_type;

    int send_res = mon_vm_send_txn_async(txn, &msg);
    if (send_res != MON_VM_STATUS_SUCCESS) {
        free_txn(state, handle.slot);
        return send_res;
    }

    int wait_res = mon_vm_wait_for_acks(handle, &msg);
    free_txn(state, handle.slot);
    return wait_res;
}

int mon_vm_send_tlb_invalidate_range(vm_space_t *space, uintptr_t va, size_t len, bool strict) {
    // Delegate strictly to the authoritative hardened TLB shootdown subsystem
    extern kstatus_t vmm_send_tlb_invalidate_ex(address_space_t *aspace,
                                                uint64_t va,
                                                uint64_t len,
                                                uint32_t type,
                                                int failure_policy);
    if (!space || !space->aspace) return MON_VM_STATUS_MALFORMED;
    return vmm_send_tlb_invalidate_ex(space->aspace, va, len, 0, 0);
}

int mon_vm_send_tlb_invalidate_all(vm_space_t *space, bool strict) {
    extern kstatus_t vmm_send_tlb_invalidate_ex(address_space_t *aspace,
                                                uint64_t va,
                                                uint64_t len,
                                                uint32_t type,
                                                int failure_policy);
    if (!space || !space->aspace) return MON_VM_STATUS_MALFORMED;
    return vmm_send_tlb_invalidate_ex(space->aspace, 0, 0, 2, 0);
}

int mon_vm_send_prepare_rt(vm_space_t *space, uint32_t target_core_id) {
    if (!space) return MON_VM_STATUS_MALFORMED;

    uint32_t my_core = hal_cpu_get_id();
    mon_vm_core_state_t *state = &g_mon_vm_core_states[my_core];
    if (!state->initialized) return MON_VM_STATUS_CHANNEL_ERROR;

    mon_vm_tx_handle_t handle;
    mon_vm_pending_txn_t *txn = alloc_txn(state, &handle);
    if (!txn) return MON_VM_STATUS_CHANNEL_ERROR;

    txn->target_mask = (1ULL << target_core_id) & ~(1ULL << my_core);
    txn->space_id = space->space_id;
    txn->expected_generation = space->generation;
    txn->proposed_generation = space->generation;
    txn->op = MON_VM_PREPARE_RT;

    mon_vm_msg_t msg = {0};
    msg.h.version_major = MON_VM_PROTO_VERSION_MAJOR;
    msg.h.version_minor = MON_VM_PROTO_VERSION_MINOR;
    msg.h.type = MON_VM_PREPARE_RT;
    msg.h.flags = MON_VM_F_STRICT_ACK | MON_VM_F_PREPARE_ONLY;
    msg.h.space_id = space->space_id;
    msg.h.expected_generation = space->generation;
    msg.h.proposed_generation = space->generation;
    msg.h.payload_len = 0;

    int send_res = mon_vm_send_txn_async(txn, &msg);
    if (send_res != MON_VM_STATUS_SUCCESS) {
        free_txn(state, handle.slot);
        return send_res;
    }

    int wait_res = mon_vm_wait_for_acks(handle, &msg);
    free_txn(state, handle.slot);
    return wait_res;
}
