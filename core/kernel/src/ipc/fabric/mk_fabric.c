#include "ipc/mk_proto.h"
#include <hal/hal.h>
#include <stdatomic.h>

static bh_mk_core_fabric_t g_core_fabrics[BHARAT_MAX_CPUS];
static uint32_t g_fabric_core_count = 0;
static _Atomic uint32_t g_pending_lanes[BHARAT_MAX_CPUS];
static const bh_mk_doorbell_ops_t *g_doorbell_ops = NULL;

// Functions implemented in separate files
kstatus_t bh_mk_mpsc_ring_init(bh_mk_mpsc_ring_t *ring, bh_mk_ring_slot_t *slots, uint32_t capacity);
kstatus_t bh_mk_mpsc_ring_enqueue(bh_mk_mpsc_ring_t *ring, const bh_mk_wire_message_t *msg);
kstatus_t bh_mk_mpsc_ring_dequeue(bh_mk_mpsc_ring_t *ring, bh_mk_wire_message_t *out_msg);

kstatus_t bh_mk_endpoint_resolve(
    bh_mk_core_fabric_t *f,
    bh_mk_endpoint_handle_t handle,
    bh_mk_endpoint_entry_t **out_entry);

kstatus_t bh_mk_tx_alloc(
    uint32_t dst_core,
    uint64_t dst_endpoint,
    uint32_t msg_class,
    uint32_t opcode,
    uint64_t deadline_ticks,
    bh_mk_tx_handle_t *out_handle);

kstatus_t bh_mk_tx_complete(
    bh_mk_tx_handle_t handle,
    uint32_t expected_source_core,
    bh_mk_endpoint_handle_t expected_source_endpoint,
    kstatus_t result);

kstatus_t bh_mk_tx_reap(bh_mk_tx_handle_t handle, kstatus_t *out_result);

kstatus_t bh_mk_replay_check_and_add(
    uint32_t src_core,
    uint64_t src_endpoint,
    uint32_t src_ep_gen,
    uint32_t txn_slot,
    uint32_t txn_gen,
    uint16_t msg_class,
    uint16_t opcode,
    uint64_t sequence,
    uint64_t timestamp);

void bh_mk_register_doorbell(const bh_mk_doorbell_ops_t *ops) {
    g_doorbell_ops = ops;
}

bh_mk_core_fabric_t *bh_mk_get_core_fabric(uint32_t core_id) {
    if (core_id >= BHARAT_MAX_CPUS) {
        return NULL;
    }
    return &g_core_fabrics[core_id];
}

kstatus_t bh_mk_fabric_init(uint32_t discovered_core_count) {
    if (discovered_core_count > BHARAT_MAX_CPUS) {
        return K_ERR_INVALID_ARG;
    }
    g_fabric_core_count = discovered_core_count;

    for (uint32_t i = 0; i < BHARAT_MAX_CPUS; i++) {
        bh_mk_core_fabric_t *f = &g_core_fabrics[i];
        __builtin_memset(f, 0, sizeof(bh_mk_core_fabric_t));

        bh_mk_mpsc_ring_init(&f->control_in, f->control_slots, BH_MK_LANE_CONTROL_CAP);
        bh_mk_mpsc_ring_init(&f->normal_in, f->normal_slots, BH_MK_LANE_NORMAL_CAP);
        bh_mk_mpsc_ring_init(&f->bulk_in, f->bulk_slots, BH_MK_LANE_BULK_CAP);

        for (uint32_t t = 0; t < BH_MK_TX_TABLE_SIZE; t++) {
            f->txns.entries[t].in_use = 0;
            f->txns.entries[t].state = BH_MK_TXN_FREE;
            f->txns.entries[t].generation = 1;
        }

        for (uint32_t e = 0; e < BH_MK_MAX_ENDPOINTS; e++) {
            f->endpoints.entries[e].bound = 0;
            f->endpoints.entries[e].generation = 1;
        }

        f->replay.head = 0;
        for (uint32_t r = 0; r < BH_MK_REPLAY_CACHE_SIZE; r++) {
            f->replay.entries[r].sequence = 0;
            f->replay.entries[r].src_core = 0xFFFFFFFFU;
            f->replay.entries[r].src_endpoint = 0xFFFFFFFFU;
            f->replay.entries[r].timestamp = 0;
        }

        atomic_store_explicit(&f->generation, 1, memory_order_relaxed);
        atomic_store_explicit(&f->ready, 1, memory_order_release);
    }

    return K_OK;
}

kstatus_t bh_mk_notify_destination(uint32_t dst_core, bh_mk_lane_t lane) {
    if (dst_core >= BHARAT_MAX_CPUS) {
        return K_ERR_INVALID_ARG;
    }
    uint32_t bit = 1U << lane;
    uint32_t prev = atomic_fetch_or_explicit(&g_pending_lanes[dst_core], bit, memory_order_acq_rel);
    if (prev == 0 && g_doorbell_ops && g_doorbell_ops->notify) {
        return g_doorbell_ops->notify(dst_core);
    }
    return K_OK;
}

static kstatus_t bh_mk_wire_validate(const bh_mk_wire_message_t *msg) {
    if (!msg) return K_ERR_INVALID_ARG;

    const bh_mk_wire_header_v1_t *h = &msg->header;

    if (h->abi_version != BH_MK_ABI_VERSION_V1) {
        return K_ERR_VERSION;
    }
    if (h->header_size != sizeof(bh_mk_wire_header_v1_t)) {
        return K_ERR_VERSION;
    }
    if (h->payload_size > BH_MK_INLINE_PAYLOAD_MAX) {
        return K_ERR_OVERFLOW;
    }
    if (h->src_core >= BHARAT_MAX_CPUS || h->dst_core >= BHARAT_MAX_CPUS) {
        return K_ERR_NET_BAD_ADDR;
    }
    if (h->reserved != 0) {
        return K_ERR_INVALID_ARG;
    }
    if (h->deadline_ticks > 0 && h->deadline_ticks < hal_timer_monotonic_ticks()) {
        return K_ERR_TIMEOUT;
    }
    if (h->txn_slot != 0xFFFFFFFFU) {
        if (h->txn_slot >= BH_MK_TX_TABLE_SIZE) {
            return K_ERR_INVALID_ARG;
        }
    }
    return K_OK;
}

kstatus_t bh_mk_send(
    bh_mk_endpoint_handle_t source,
    bh_mk_endpoint_handle_t destination,
    uint16_t message_class,
    uint16_t opcode,
    bh_mk_lane_t lane,
    const void *payload,
    uint32_t payload_size,
    bh_mk_tx_handle_t *out_transaction)
{
    if (payload_size > 0 && !payload) {
        return K_ERR_INVALID_ARG;
    }
    if (payload_size > BH_MK_INLINE_PAYLOAD_MAX) {
        return K_ERR_OVERFLOW;
    }

    uint32_t src_core = hal_cpu_get_id();
    bh_mk_core_fabric_t *src_fab = bh_mk_get_core_fabric(src_core);
    if (!src_fab) {
        return K_ERR_BAD_STATE;
    }

    bh_mk_endpoint_entry_t *src_ep_entry = NULL;
    uint32_t src_ep_gen = 0;
    if (source != BH_MK_ENDPOINT_LEGACY) {
        kstatus_t resolve_status = bh_mk_endpoint_resolve(src_fab, source, &src_ep_entry);
        if (resolve_status != K_OK) {
            return resolve_status;
        }
        src_ep_gen = src_ep_entry->generation;
    }

    uint32_t dst_core = 0;
    uint32_t dst_ep_slot = 0;
    uint32_t dst_ep_gen = 0;
    uint32_t expected_core_gen = 0;

    if (destination == BH_MK_ENDPOINT_LEGACY) {
        dst_core = 0;
        dst_ep_slot = (uint32_t)BH_MK_ENDPOINT_LEGACY;
        dst_ep_gen = 0;
    } else if ((destination & 0xFFFFFFFFFFFFFFULL) == BH_MK_ENDPOINT_LEGACY) {
        dst_core = (destination >> 24) & 0xFFU;
        dst_ep_slot = (uint32_t)BH_MK_ENDPOINT_LEGACY;
        dst_ep_gen = 0;
    } else {
        uint32_t dummy_slot;
        kstatus_t unpack_status = bh_mk_handle_unpack(destination, &dst_core, &expected_core_gen, &dst_ep_gen, &dummy_slot);
        if (unpack_status != K_OK) {
            return unpack_status;
        }
        dst_ep_slot = dummy_slot;
    }

    bh_mk_core_fabric_t *dst_fab = bh_mk_get_core_fabric(dst_core);
    if (!dst_fab || !atomic_load_explicit(&dst_fab->ready, memory_order_acquire)) {
        src_fab->diagnostics.credit_would_blocks++;
        return K_ERR_DEV_OFFLINE;
    }

    uint32_t gen_before = atomic_load_explicit(&dst_fab->generation, memory_order_acquire);
    if (destination != BH_MK_ENDPOINT_LEGACY && ((destination & 0xFFFFFFFFFFFFFFULL) != BH_MK_ENDPOINT_LEGACY)) {
        if (expected_core_gen != (gen_before & 0xFFU)) {
            src_fab->diagnostics.tx_errors++;
            return K_ERR_STALE;
        }
    }

    bh_mk_tx_handle_t tx_handle = {0};
    uint32_t tx_allocated = 0;

    if (out_transaction) {
        uint64_t ticks = hal_timer_monotonic_ticks();
        kstatus_t tx_status = bh_mk_tx_alloc(dst_core, destination, message_class, opcode, ticks + 1000, &tx_handle);
        if (tx_status != K_OK) {
            return tx_status;
        }
        tx_allocated = 1;
    }

    bh_mk_wire_message_t wire_msg = {0};
    wire_msg.header.abi_version = BH_MK_ABI_VERSION_V1;
    wire_msg.header.header_size = sizeof(bh_mk_wire_header_v1_t);
    wire_msg.header.message_class = message_class;
    wire_msg.header.opcode = opcode;
    wire_msg.header.src_core = src_core;
    wire_msg.header.dst_core = dst_core;
    wire_msg.header.src_endpoint = source;
    wire_msg.header.src_endpoint_generation = src_ep_gen;
    wire_msg.header.dst_endpoint = destination;
    wire_msg.header.dst_endpoint_generation = dst_ep_gen;

    if (tx_allocated) {
        wire_msg.header.txn_slot = tx_handle.slot;
        wire_msg.header.txn_generation = tx_handle.generation;
    } else {
        wire_msg.header.txn_slot = 0xFFFFFFFFU;
        wire_msg.header.txn_generation = 0xFFFFFFFFU;
    }

    static _Atomic uint64_t g_sequence_counter = 1;
    wire_msg.header.sequence = atomic_fetch_add_explicit(&g_sequence_counter, 1, memory_order_relaxed);
    wire_msg.header.deadline_ticks = hal_timer_monotonic_ticks() + 1000;
    wire_msg.header.payload_size = payload_size;

    if (payload_size > 0) {
        __builtin_memcpy(wire_msg.payload, payload, payload_size);
    }

    bh_mk_mpsc_ring_t *ring = NULL;
    if (lane == BH_MK_LANE_CONTROL) {
        ring = &dst_fab->control_in;
    } else if (lane == BH_MK_LANE_NORMAL) {
        ring = &dst_fab->normal_in;
    } else {
        ring = &dst_fab->bulk_in;
    }

    kstatus_t enqueue_status = bh_mk_mpsc_ring_enqueue(ring, &wire_msg);
    if (enqueue_status != K_OK) {
        if (tx_allocated) {
            kstatus_t dummy;
            bh_mk_tx_reap(tx_handle, &dummy);
        }
        src_fab->diagnostics.tx_errors++;
        return enqueue_status;
    }

    uint32_t gen_after = atomic_load_explicit(&dst_fab->generation, memory_order_acquire);
    if (gen_before != gen_after) {
        if (tx_allocated) {
            kstatus_t dummy;
            bh_mk_tx_reap(tx_handle, &dummy);
        }
        src_fab->diagnostics.tx_errors++;
        return K_ERR_STALE;
    }

    src_fab->diagnostics.tx_messages++;

    bh_mk_notify_destination(dst_core, lane);

    if (out_transaction) {
        *out_transaction = tx_handle;
    }
    return K_OK;
}

static void bh_mk_send_error_reply(const bh_mk_wire_message_t *orig_msg, kstatus_t error_code) {
    bh_mk_wire_message_t reply = {0};
    reply.header.abi_version = BH_MK_ABI_VERSION_V1;
    reply.header.header_size = sizeof(bh_mk_wire_header_v1_t);
    reply.header.message_class = orig_msg->header.message_class;
    reply.header.opcode = orig_msg->header.opcode;
    reply.header.flags = MK_MSG_FLAG_IS_REPLY | MK_MSG_FLAG_ERROR;
    reply.header.src_core = hal_cpu_get_id();
    reply.header.dst_core = orig_msg->header.src_core;
    reply.header.src_endpoint = orig_msg->header.dst_endpoint;
    reply.header.dst_endpoint = orig_msg->header.src_endpoint;
    reply.header.txn_slot = orig_msg->header.txn_slot;
    reply.header.txn_generation = orig_msg->header.txn_generation;
    reply.header.sequence = 0;
    reply.header.deadline_ticks = hal_timer_monotonic_ticks() + 1000;
    reply.header.payload_size = 0;

    bh_mk_core_fabric_t *dst_fab = bh_mk_get_core_fabric(orig_msg->header.src_core);
    if (dst_fab) {
        bh_mk_mpsc_ring_enqueue(&dst_fab->control_in, &reply);
        bh_mk_notify_destination(orig_msg->header.src_core, BH_MK_LANE_CONTROL);
    }
}

static kstatus_t bh_mk_dispatch_internal(bh_mk_core_fabric_t *f, const bh_mk_wire_message_t *msg) {
    kstatus_t val_status = bh_mk_wire_validate(msg);
    if (val_status != K_OK) {
        f->diagnostics.rx_errors++;
        return val_status;
    }

    f->diagnostics.rx_messages++;

    kstatus_t replay_status = bh_mk_replay_check_and_add(
        msg->header.src_core,
        msg->header.src_endpoint,
        msg->header.src_endpoint_generation,
        msg->header.txn_slot,
        msg->header.txn_generation,
        msg->header.message_class,
        msg->header.opcode,
        msg->header.sequence,
        hal_timer_monotonic_ticks());

    if (replay_status != K_OK) {
        f->diagnostics.rx_errors++;
        return replay_status; // Duplicate message dropped
    }

    if (msg->header.flags & MK_MSG_FLAG_IS_REPLY) {
        bh_mk_tx_handle_t tx_handle = {
            .slot = msg->header.txn_slot,
            .generation = msg->header.txn_generation
        };
        kstatus_t comp_status = bh_mk_tx_complete(tx_handle, msg->header.src_core, msg->header.src_endpoint, K_OK);
        if (comp_status != K_OK) {
            f->diagnostics.rx_errors++;
        }
        return comp_status;
    }

    if (msg->header.dst_endpoint == BH_MK_ENDPOINT_LEGACY || ((msg->header.dst_endpoint & 0xFFFFFFFFFFFFFFULL) == BH_MK_ENDPOINT_LEGACY)) {
        extern int mk_dispatch_legacy_adapter(const bh_mk_wire_message_t *m);
        kstatus_t leg_status = mk_dispatch_legacy_adapter(msg);
        if (leg_status != K_OK) {
            f->diagnostics.rx_errors++;
        }
        return leg_status;
    }

    bh_mk_endpoint_entry_t *ep = NULL;
    kstatus_t resolve_status = bh_mk_endpoint_resolve(f, msg->header.dst_endpoint, &ep);
    if (resolve_status != K_OK) {
        f->diagnostics.rx_errors++;
        if (msg->header.txn_slot != 0xFFFFFFFFU) {
            bh_mk_send_error_reply(msg, K_ERR_CAP_STALE);
        }
        return resolve_status;
    }

    if (ep->handler_fn) {
        kstatus_t handler_status = ep->handler_fn(msg->header.src_endpoint, msg, ep->ctx);
        if (handler_status != K_OK) {
            f->diagnostics.rx_errors++;
        }
        return handler_status;
    }

    return K_OK;
}

kstatus_t bh_mk_drain_local(uint32_t budget) {
    uint32_t core_id = hal_cpu_get_id();
    bh_mk_core_fabric_t *f = bh_mk_get_core_fabric(core_id);
    if (!f || !atomic_load_explicit(&f->ready, memory_order_acquire)) {
        return K_ERR_BAD_STATE;
    }

    atomic_exchange_explicit(&g_pending_lanes[core_id], 0, memory_order_acq_rel);

    uint32_t processed = 0;
    bh_mk_wire_message_t msg;

    uint32_t control_limit = (budget * 5) / 8;
    if (control_limit < 4) control_limit = 4;
    uint32_t normal_limit = (budget * 3) / 8;
    if (normal_limit < 2) normal_limit = 2;
    uint32_t bulk_limit = budget - control_limit - normal_limit;
    if (bulk_limit < 1) bulk_limit = 1;

    uint32_t control_count = 0;
    uint32_t normal_count = 0;
    uint32_t bulk_count = 0;

    while (control_count < control_limit && processed < budget) {
        if (bh_mk_mpsc_ring_dequeue(&f->control_in, &msg) == K_OK) {
            bh_mk_dispatch_internal(f, &msg);
            control_count++;
            processed++;
        } else {
            break;
        }
    }

    while (normal_count < normal_limit && processed < budget) {
        if (bh_mk_mpsc_ring_dequeue(&f->normal_in, &msg) == K_OK) {
            bh_mk_dispatch_internal(f, &msg);
            normal_count++;
            processed++;
        } else {
            break;
        }
    }

    while (bulk_count < bulk_limit && processed < budget) {
        if (bh_mk_mpsc_ring_dequeue(&f->bulk_in, &msg) == K_OK) {
            bh_mk_dispatch_internal(f, &msg);
            bulk_count++;
            processed++;
        } else {
            break;
        }
    }

    while (processed < budget) {
        if (bh_mk_mpsc_ring_dequeue(&f->control_in, &msg) == K_OK) {
            bh_mk_dispatch_internal(f, &msg);
            processed++;
        } else if (bh_mk_mpsc_ring_dequeue(&f->normal_in, &msg) == K_OK) {
            bh_mk_dispatch_internal(f, &msg);
            processed++;
        } else if (bh_mk_mpsc_ring_dequeue(&f->bulk_in, &msg) == K_OK) {
            bh_mk_dispatch_internal(f, &msg);
            processed++;
        } else {
            break;
        }
    }

    return K_OK;
}
