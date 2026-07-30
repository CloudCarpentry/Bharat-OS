#include "monitor/mon_vm_ops.h"
#include "mm/vm_space.h"
#include "mm/aspace.h"
#include "mm/prot_domain.h"
#include "hal/hal.h"
#include "urpc/urpc_bootstrap.h"
#include "mon_vm_state.h"
#include <stddef.h>

#ifdef BHARAT_HOST_TEST
#include <string.h>
#else
void *memcpy(void *dest, const void *src, size_t n);
void *memset(void *s, int c, size_t n);
#endif

// Forward declarations for multi-slot inbox publishing/reservation
extern int mon_vm_reserve_inbox_slot(uint32_t dst_core, uint32_t *out_slot_idx, uint32_t *out_slot_gen);
extern void mon_vm_publish_inbox_slot(uint32_t dst_core, uint32_t slot_idx, const mon_vm_msg_t *msg);
extern void mon_vm_decode_doorbell(uint64_t token, uint8_t *out_class, uint8_t *out_origin, uint16_t *out_slot, uint32_t *out_slot_gen);

// Active space lookup table inside host or kernel
extern vm_space_t* g_active_spaces[128]; // Pre-allocated mock database

// Find realized space
static vm_space_t* find_realized_space(uint64_t space_id) {
    for (int i = 0; i < 128; i++) {
        if (g_active_spaces[i] && g_active_spaces[i]->space_id == space_id) {
            return g_active_spaces[i];
        }
    }
    return NULL;
}

static void send_ack_or_nack(const mon_vm_hdr_t *hdr, int32_t status) {
    if (!(hdr->flags & MON_VM_F_STRICT_ACK)) return;

    uint32_t my_core = hal_cpu_get_id();
    mon_vm_core_state_t *state = &g_mon_vm_core_states[my_core];

    mon_vm_msg_t ack = {0};
    ack.ack.h.version_major = MON_VM_PROTO_VERSION_MAJOR;
    ack.ack.h.version_minor = MON_VM_PROTO_VERSION_MINOR;
    ack.ack.h.type = (status == MON_VM_STATUS_SUCCESS) ? MON_VM_ACK : MON_VM_NACK;
    ack.ack.h.space_id = hdr->space_id;
    ack.ack.h.expected_generation = hdr->expected_generation;
    ack.ack.h.proposed_generation = hdr->proposed_generation;
    ack.ack.h.src_core = my_core;
    ack.ack.h.dst_core = hdr->src_core;
    ack.ack.h.tx_origin_core = hdr->tx_origin_core;
    ack.ack.h.tx_slot = hdr->tx_slot;
    ack.ack.h.tx_generation = hdr->tx_generation;
    ack.ack.status = status;
    ack.ack.realize_state = 1;

    // Reserve destination slot (0 = FREE -> 1 = WRITING)
    uint32_t slot_idx = 0;
    uint32_t slot_gen = 0;
    int res = mon_vm_reserve_inbox_slot(hdr->src_core, &slot_idx, &slot_gen);
    if (res != MON_VM_STATUS_SUCCESS) {
        // Destination inbox is full. In real kernel we would retry/block.
        return;
    }

    // Publish to destination slot (copies data, stores READY (2) with release semantics)
    mon_vm_publish_inbox_slot(hdr->src_core, slot_idx, &ack);

    // Send doorbell token via urpc
    uint64_t doorbell = ((uint64_t)0x99 << 56) |
                       ((uint64_t)my_core << 48) |
                       ((uint64_t)slot_idx << 32) |
                       (uint64_t)slot_gen;

    urpc_bootstrap_send(hdr->src_core, doorbell);

    spin_lock(&state->lock);
    state->stat_acks_sent++;
    spin_unlock(&state->lock);
}

static int handle_map(mon_vm_map_msg_t *msg, size_t len) {
    if (len < sizeof(mon_vm_map_msg_t)) return MON_VM_STATUS_MALFORMED;
    if (msg->length == 0 || (msg->va_start & 0xFFF) || (msg->pa_start & 0xFFF)) {
        send_ack_or_nack(&msg->h, MON_VM_STATUS_MALFORMED);
        return MON_VM_STATUS_MALFORMED;
    }

    uint32_t my_core = hal_cpu_get_id();
    mon_vm_core_state_t *state = &g_mon_vm_core_states[my_core];

    int32_t cached_status = 0;
    if (mon_vm_check_replay(msg->h.tx_origin_core, msg->h.tx_slot, msg->h.tx_generation, MON_VM_MAP, &cached_status)) {
        send_ack_or_nack(&msg->h, cached_status);
        return cached_status;
    }

    vm_space_t *space = find_realized_space(msg->h.space_id);
    if (!space || !space->aspace) {
        send_ack_or_nack(&msg->h, MON_VM_STATUS_UNSUPPORTED);
        return MON_VM_STATUS_UNSUPPORTED;
    }

    // Direct hardware dispatch through canonical protection domain layer
    prot_domain_t *pd = space->aspace->prot_domain;
    int res = prot_domain_map_region(pd, msg->va_start, msg->pa_start, msg->length, msg->prot);
    int32_t final_res = (res == 0) ? MON_VM_STATUS_SUCCESS : MON_VM_STATUS_UNSUPPORTED;

    // Track statefully
    spin_lock(&state->lock);
    state->realized_spaces_mask |= (1ULL << msg->h.space_id);
    spin_unlock(&state->lock);

    mon_vm_record_replay(msg->h.tx_origin_core, msg->h.tx_slot, msg->h.tx_generation, MON_VM_MAP, final_res);
    send_ack_or_nack(&msg->h, final_res);
    return final_res;
}

static int handle_unmap(mon_vm_unmap_msg_t *msg, size_t len) {
    if (len < sizeof(mon_vm_unmap_msg_t)) return MON_VM_STATUS_MALFORMED;
    if (msg->length == 0 || (msg->va_start & 0xFFF)) {
        send_ack_or_nack(&msg->h, MON_VM_STATUS_MALFORMED);
        return MON_VM_STATUS_MALFORMED;
    }

    int32_t cached_status = 0;
    if (mon_vm_check_replay(msg->h.tx_origin_core, msg->h.tx_slot, msg->h.tx_generation, MON_VM_UNMAP, &cached_status)) {
        send_ack_or_nack(&msg->h, cached_status);
        return cached_status;
    }

    vm_space_t *space = find_realized_space(msg->h.space_id);
    if (!space || !space->aspace) {
        send_ack_or_nack(&msg->h, MON_VM_STATUS_UNSUPPORTED);
        return MON_VM_STATUS_UNSUPPORTED;
    }

    prot_domain_t *pd = space->aspace->prot_domain;
    int res = prot_domain_unmap_region(pd, msg->va_start, msg->length);
    int32_t final_res = (res == 0) ? MON_VM_STATUS_SUCCESS : MON_VM_STATUS_UNSUPPORTED;

    if (final_res == MON_VM_STATUS_SUCCESS) {
        uint32_t my_core = hal_cpu_get_id();
        mon_vm_core_state_t *state = &g_mon_vm_core_states[my_core];
        spin_lock(&state->lock);
        state->realized_spaces_mask &= ~(1ULL << msg->h.space_id);
        spin_unlock(&state->lock);
    }

    mon_vm_record_replay(msg->h.tx_origin_core, msg->h.tx_slot, msg->h.tx_generation, MON_VM_UNMAP, final_res);
    send_ack_or_nack(&msg->h, final_res);
    return final_res;
}

static int handle_protect(mon_vm_protect_msg_t *msg, size_t len) {
    if (len < sizeof(mon_vm_protect_msg_t)) return MON_VM_STATUS_MALFORMED;
    if (msg->length == 0 || (msg->va_start & 0xFFF)) {
        send_ack_or_nack(&msg->h, MON_VM_STATUS_MALFORMED);
        return MON_VM_STATUS_MALFORMED;
    }

    int32_t cached_status = 0;
    if (mon_vm_check_replay(msg->h.tx_origin_core, msg->h.tx_slot, msg->h.tx_generation, MON_VM_PROTECT, &cached_status)) {
        send_ack_or_nack(&msg->h, cached_status);
        return cached_status;
    }

    vm_space_t *space = find_realized_space(msg->h.space_id);
    if (!space || !space->aspace) {
        send_ack_or_nack(&msg->h, MON_VM_STATUS_UNSUPPORTED);
        return MON_VM_STATUS_UNSUPPORTED;
    }

    prot_domain_t *pd = space->aspace->prot_domain;
    int res = prot_domain_protect_region(pd, msg->va_start, msg->length, msg->prot);
    int32_t final_res = (res == 0) ? MON_VM_STATUS_SUCCESS : MON_VM_STATUS_UNSUPPORTED;

    mon_vm_record_replay(msg->h.tx_origin_core, msg->h.tx_slot, msg->h.tx_generation, MON_VM_PROTECT, final_res);
    send_ack_or_nack(&msg->h, final_res);
    return final_res;
}

static int handle_space_destroy(mon_vm_hdr_t *msg, size_t len) {
    (void)len;
    uint32_t my_core = hal_cpu_get_id();
    mon_vm_core_state_t *state = &g_mon_vm_core_states[my_core];

    int32_t cached_status = 0;
    if (mon_vm_check_replay(msg->tx_origin_core, msg->tx_slot, msg->tx_generation, MON_VM_SPACE_DESTROY, &cached_status)) {
        send_ack_or_nack(msg, cached_status);
        return cached_status;
    }

    vm_space_t *space = find_realized_space(msg->space_id);
    if (!space || !space->aspace) {
        send_ack_or_nack(msg, MON_VM_STATUS_UNSUPPORTED);
        return MON_VM_STATUS_UNSUPPORTED;
    }

    // Direct hardware teardown
    prot_domain_t *pd = space->aspace->prot_domain;
    if (pd) {
        prot_domain_destroy(pd);
        space->aspace->prot_domain = NULL;
    }

    spin_lock(&state->lock);
    state->realized_spaces_mask &= ~(1ULL << msg->space_id);
    spin_unlock(&state->lock);

    mon_vm_record_replay(msg->tx_origin_core, msg->tx_slot, msg->tx_generation, MON_VM_SPACE_DESTROY, MON_VM_STATUS_SUCCESS);
    send_ack_or_nack(msg, MON_VM_STATUS_SUCCESS);
    return MON_VM_STATUS_SUCCESS;
}

static int handle_ack(mon_vm_ack_msg_t *msg, size_t len) {
    if (len < sizeof(mon_vm_ack_msg_t)) return MON_VM_STATUS_MALFORMED;

    uint32_t my_core = hal_cpu_get_id();
    mon_vm_core_state_t *state = &g_mon_vm_core_states[my_core];

    spin_lock(&state->lock);
    state->stat_acks_received++;

    uint16_t slot = msg->h.tx_slot;
    if (slot < MAX_PENDING_TXNS) {
        mon_vm_pending_txn_t *txn = &state->pending_txns[slot];
        if (txn->in_use && txn->handle.generation == msg->h.tx_generation) {
            txn->ack_mask |= (1ULL << msg->h.src_core);
            if (msg->status == MON_VM_STATUS_SUCCESS) {
                txn->success_mask |= (1ULL << msg->h.src_core);
            } else {
                txn->nack_mask |= (1ULL << msg->h.src_core);
                txn->final_status = msg->status;
            }
        }
    }
    spin_unlock(&state->lock);

    return 0;
}

int mon_vm_dispatch(void *raw_msg, size_t len) {
    if (!raw_msg || len < sizeof(mon_vm_hdr_t)) return MON_VM_STATUS_MALFORMED;

    uint32_t my_core = hal_cpu_get_id();
    mon_vm_core_state_t *state = &g_mon_vm_core_states[my_core];

    spin_lock(&state->lock);
    state->stat_requests_received++;
    spin_unlock(&state->lock);

    mon_vm_hdr_t *hdr = (mon_vm_hdr_t *)raw_msg;
    switch (hdr->type) {
        case MON_VM_MAP:
            return handle_map((mon_vm_map_msg_t *)raw_msg, len);
        case MON_VM_UNMAP:
            return handle_unmap((mon_vm_unmap_msg_t *)raw_msg, len);
        case MON_VM_PROTECT:
            return handle_protect((mon_vm_protect_msg_t *)raw_msg, len);
        case MON_VM_SPACE_DESTROY:
            return handle_space_destroy(hdr, len);
        case MON_VM_ACK:
        case MON_VM_NACK:
            return handle_ack((mon_vm_ack_msg_t *)raw_msg, len);
        default:
            send_ack_or_nack(hdr, MON_VM_STATUS_UNSUPPORTED);
            return MON_VM_STATUS_UNSUPPORTED;
    }

    return 0;
}

// Bounded multi-slot inbox polling loop using robust release-acquire atomics
void mon_vm_poll_inbox(void) {
    uint32_t my_core = hal_cpu_get_id();
    mon_vm_core_state_t *state = &g_mon_vm_core_states[my_core];
    if (!state->initialized) return;

    for (int i = 0; i < MON_VM_INBOX_SLOTS; i++) {
        // Load slot state with acquire memory ordering
        uint32_t slot_state = __atomic_load_n(&state->inbox[i].state, __ATOMIC_ACQUIRE);
        if (slot_state == 2) { // READY
            // Atomically acquire reading ownership (READY (2) -> READING (3)) using CAS
            if (__sync_bool_compare_and_swap(&state->inbox[i].state, 2, 3)) {

                // Copy full frame payload to receiver-local buffer
                mon_vm_msg_t local_msg;
                memcpy(&local_msg, &state->inbox[i].msg, sizeof(mon_vm_msg_t));

                // Release inbox slot (incrementing slot generation to prevent ABA)
                state->inbox[i].generation++;
                __atomic_store_n(&state->inbox[i].state, 0, __ATOMIC_RELEASE); // 0 = FREE

                // Safely dispatch the copied local message
                mon_vm_dispatch(&local_msg, sizeof(mon_vm_msg_t));
            }
        }
    }
}
