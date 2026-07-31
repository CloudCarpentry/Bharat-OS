#include "ipc/mk_proto.h"
#include <hal/hal.h>

kstatus_t bh_mk_tx_alloc(
    uint32_t dst_core,
    uint64_t dst_endpoint,
    uint32_t msg_class,
    uint32_t opcode,
    uint64_t deadline_ticks,
    bh_mk_tx_handle_t *out_handle)
{
    if (!out_handle) {
        return K_ERR_INVALID_ARG;
    }

    uint32_t core_id = hal_cpu_get_id();
    bh_mk_core_fabric_t *f = bh_mk_get_core_fabric(core_id);
    if (!f) {
        return K_ERR_BAD_STATE;
    }

    for (uint32_t i = 0; i < BH_MK_TX_TABLE_SIZE; i++) {
        bh_mk_tx_entry_t *entry = &f->txns.entries[i];

        // Atomically claim the slot
        uint8_t expected = 0;
        if (atomic_compare_exchange_strong_explicit(&entry->in_use, &expected, 1, memory_order_acquire, memory_order_relaxed)) {
            entry->state = BH_MK_TXN_RESERVED;
            entry->dst_core = dst_core;
            entry->dst_endpoint = dst_endpoint;
            entry->msg_class = msg_class;
            entry->opcode = opcode;
            entry->deadline_ticks = deadline_ticks;
            entry->result = K_OK;
            entry->legacy_txn_id = 0;

            out_handle->slot = i;
            out_handle->generation = entry->generation;
            return K_OK;
        }
    }

    return K_ERR_NO_RESOURCES;
}

kstatus_t bh_mk_tx_complete(
    bh_mk_tx_handle_t handle,
    uint32_t expected_source_core,
    bh_mk_endpoint_handle_t expected_source_endpoint,
    kstatus_t result)
{
    uint32_t core_id = hal_cpu_get_id();
    bh_mk_core_fabric_t *f = bh_mk_get_core_fabric(core_id);
    if (!f) {
        return K_ERR_BAD_STATE;
    }

    if (handle.slot >= BH_MK_TX_TABLE_SIZE) {
        return K_ERR_INVALID_ARG;
    }

    bh_mk_tx_entry_t *entry = &f->txns.entries[handle.slot];
    if (!atomic_load_explicit(&entry->in_use, memory_order_relaxed) || entry->generation != handle.generation) {
        return K_ERR_CAP_STALE;
    }

    // Verify expected source core matches destination core
    if (entry->dst_core != expected_source_core) {
        return K_ERR_DENIED;
    }

    // Verify expected source endpoint matches destination endpoint
    if (entry->dst_endpoint != expected_source_endpoint) {
        return K_ERR_DENIED;
    }

    if (result == K_OK) {
        entry->state = BH_MK_TXN_ACKED;
    } else if (result == K_ERR_TIMEOUT) {
        entry->state = BH_MK_TXN_TIMED_OUT;
    } else {
        entry->state = BH_MK_TXN_CANCELLED;
    }
    entry->result = result;

    return K_OK;
}

kstatus_t bh_mk_tx_reap(bh_mk_tx_handle_t handle, kstatus_t *out_result) {
    uint32_t core_id = hal_cpu_get_id();
    bh_mk_core_fabric_t *f = bh_mk_get_core_fabric(core_id);
    if (!f) {
        return K_ERR_BAD_STATE;
    }

    if (handle.slot >= BH_MK_TX_TABLE_SIZE) {
        return K_ERR_INVALID_ARG;
    }

    bh_mk_tx_entry_t *entry = &f->txns.entries[handle.slot];
    if (!atomic_load_explicit(&entry->in_use, memory_order_relaxed) || entry->generation != handle.generation) {
        return K_ERR_CAP_STALE;
    }

    if (out_result) {
        *out_result = entry->result;
    }

    entry->state = BH_MK_TXN_FREE;
    entry->generation++; // Increment generation for ABA prevention
    atomic_store_explicit(&entry->in_use, 0, memory_order_release);

    return K_OK;
}
