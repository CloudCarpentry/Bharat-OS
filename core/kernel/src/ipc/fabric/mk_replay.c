#include "ipc/mk_proto.h"
#include <hal/hal.h>

kstatus_t bh_mk_replay_check_and_add(
    uint32_t src_core,
    uint64_t src_endpoint,
    uint32_t src_ep_gen,
    uint32_t txn_slot,
    uint32_t txn_gen,
    uint16_t msg_class,
    uint16_t opcode,
    uint64_t sequence,
    uint64_t timestamp)
{
    uint32_t core_id = hal_cpu_get_id();
    bh_mk_core_fabric_t *f = bh_mk_get_core_fabric(core_id);
    if (!f) {
        return K_ERR_BAD_STATE;
    }

    // Check if duplicate using all fields of the multi-provenance key
    for (uint32_t i = 0; i < BH_MK_REPLAY_CACHE_SIZE; i++) {
        bh_mk_replay_entry_t *entry = &f->replay.entries[i];
        if (entry->src_core == src_core &&
            entry->src_endpoint == src_endpoint &&
            entry->src_endpoint_gen == src_ep_gen &&
            entry->txn_slot == txn_slot &&
            entry->txn_gen == txn_gen &&
            entry->msg_class == msg_class &&
            entry->opcode == opcode &&
            entry->sequence == sequence)
        {
            return K_ERR_ALREADY_EXISTS; // Replay detected
        }
    }

    // Insert into circular cache
    uint32_t h = f->replay.head;
    bh_mk_replay_entry_t *entry = &f->replay.entries[h];
    entry->src_core = src_core;
    entry->src_endpoint = src_endpoint;
    entry->src_endpoint_gen = src_ep_gen;
    entry->txn_slot = txn_slot;
    entry->txn_gen = txn_gen;
    entry->msg_class = msg_class;
    entry->opcode = opcode;
    entry->sequence = sequence;
    entry->timestamp = timestamp;

    f->replay.head = (h + 1) % BH_MK_REPLAY_CACHE_SIZE;

    return K_OK;
}
