#include "../../include/advanced/multikernel.h"
#include "../../include/atomic.h"
#include "../../include/hal/hal.h"
#include "../../include/kernel_safety.h"
#include "../../include/multicore.h"

// @cite The Multikernel: A New OS Architecture for Scalable Multicore Systems (Baumann et al., 2009)
// Barrelfish-inspired URPC messaging and state replication
#include <stddef.h>
#include <stdint.h>
#include <stdatomic.h>

#define MK_MAX_CHANNELS 16U
#define MK_MAX_PAYLOAD_WORDS 8U
#define MK_MSG_TYPE_AI_SUGGESTION 1U
#define MK_MAX_CORES 8U
#define MK_DEFAULT_RING_CAPACITY 32U

typedef struct {
    urpc_msg_t backing_buffer[MK_DEFAULT_RING_CAPACITY];
    urpc_ring_t ring;
    mk_channel_t channel;
    uint8_t in_use;
} mk_channel_slot_t;

static mk_channel_slot_t g_channels[MK_MAX_CHANNELS];
static mk_channel_t g_core_channel_matrix[MK_MAX_CORES][MK_MAX_CORES];
static uint8_t g_core_channel_valid[MK_MAX_CORES][MK_MAX_CORES];

static int urpc_ring_config_valid(const urpc_ring_t* ring) {
    return (ring != NULL && ring->buffer != NULL && ring->capacity >= 2U);
}

int urpc_init_ring(urpc_ring_t* ring, urpc_msg_t* buffer_ptr, uint32_t ring_size) {
    if (!ring || !buffer_ptr || ring_size < 2U) {
        return URPC_ERR_INVAL;
    }

    ring->buffer = buffer_ptr;
    ring->capacity = ring_size;
    atomic_store_explicit(&ring->head, 0U, memory_order_relaxed);
    atomic_store_explicit(&ring->tail, 0U, memory_order_relaxed);
    return URPC_SUCCESS;
}

int urpc_send(urpc_ring_t* ring, const urpc_msg_t* msg) {
    if (!urpc_ring_config_valid(ring) || !msg) {
        return URPC_ERR_INVAL;
    }

    if (msg->payload_size > sizeof(msg->payload_data)) {
        return URPC_ERR_INVAL;
    }

    // Producer uniquely owns head, so relaxed load is safe locally.
    uint32_t head = atomic_load_explicit(&ring->head, memory_order_relaxed);
    uint32_t next_head = (head + 1U) % ring->capacity;

    // Must acquire the tail to ensure previous consumer reads are visible before we overwrite the slot.
    uint32_t tail = atomic_load_explicit(&ring->tail, memory_order_acquire);
    if (next_head == tail) {
        return URPC_ERR_FULL;
    }

    ring->buffer[head] = *msg; // Normal store; payload is not published to consumer yet.

    // Publish head with release semantics. This ensures the payload store is visible
    // to the consumer when they acquire this new head value.
    atomic_store_explicit(&ring->head, next_head, memory_order_release);

    if (msg->payload_size <= sizeof(uint64_t)) {
        hal_send_ipi_payload(0U, msg->payload_data[0]);
    }

    return URPC_SUCCESS;
}

int urpc_receive(urpc_ring_t* ring, urpc_msg_t* out_msg) {
    if (!urpc_ring_config_valid(ring) || !out_msg) {
        return URPC_ERR_INVAL;
    }

    // Consumer uniquely owns tail, relaxed load is safe locally.
    uint32_t tail = atomic_load_explicit(&ring->tail, memory_order_relaxed);

    // Must acquire head to ensure the producer's payload writes are visible to us.
    uint32_t head = atomic_load_explicit(&ring->head, memory_order_acquire);

    if (tail == head) {
        return URPC_ERR_EMPTY;
    }

    *out_msg = ring->buffer[tail]; // Normal read; safely visible due to the acquire above.
    uint32_t next_tail = (tail + 1U) % ring->capacity;

    // Publish tail with release semantics. This ensures our payload read is complete
    // before the producer can see this new tail and overwrite the slot.
    atomic_store_explicit(&ring->tail, next_tail, memory_order_release);

    return URPC_SUCCESS;
}

int mk_boot_secondary_cores(uint32_t core_count) {
    if (core_count > MK_MAX_CORES) {
        return URPC_ERR_INVALID;
    }

    return multicore_boot_secondary_cores(core_count);
}

int mk_init_per_core_channels(uint32_t core_count, uint32_t ring_size) {
    if (core_count == 0U || core_count > MK_MAX_CORES || ring_size < 2U || ring_size > MK_DEFAULT_RING_CAPACITY) {
        return URPC_ERR_INVALID;
    }

    for (uint32_t s = 0; s < core_count; ++s) {
        for (uint32_t r = 0; r < core_count; ++r) {
            if (s == r) {
                g_core_channel_valid[s][r] = 0U;
                continue;
            }

            mk_channel_t* c = &g_core_channel_matrix[s][r];
            mk_channel_slot_t* slot = NULL;
            for (uint32_t i = 0; i < MK_MAX_CHANNELS; ++i) {
                if (g_channels[i].in_use == 0U) {
                    slot = &g_channels[i];
                    slot->in_use = 1U;
                    break;
                }
            }
            if (!slot) {
                return URPC_ERR_FULL;
            }

            if (urpc_init_ring(&slot->ring, slot->backing_buffer, ring_size) != URPC_SUCCESS) {
                return URPC_ERR_INVALID;
            }

            slot->channel.sender_core_id = s;
            slot->channel.receiver_core_id = r;
            slot->channel.urpc_ring = &slot->ring;
            slot->channel.ring_size = ring_size;

            *c = slot->channel;
            g_core_channel_valid[s][r] = 1U;
        }
    }

    return URPC_SUCCESS;
}

int mk_get_channel(uint32_t sender_core, uint32_t receiver_core, mk_channel_t* out_channel) {
    if (!out_channel || sender_core >= MK_MAX_CORES || receiver_core >= MK_MAX_CORES) {
        return URPC_ERR_INVALID;
    }

    if (g_core_channel_valid[sender_core][receiver_core] == 0U) {
        return URPC_ERR_NO_CHANNEL;
    }

    *out_channel = g_core_channel_matrix[sender_core][receiver_core];
    return URPC_SUCCESS;
}

int mk_establish_channel(uint32_t target_core, mk_channel_t *out_channel) {
    if (!out_channel) {
        return URPC_ERR_INVALID;
    }

    if (target_core < MK_MAX_CORES && g_core_channel_valid[0U][target_core] != 0U) {
        *out_channel = g_core_channel_matrix[0U][target_core];
        return URPC_SUCCESS;
    }

    for (uint32_t i = 0; i < MK_MAX_CHANNELS; ++i) {
        if (g_channels[i].in_use == 0U) {
            if (urpc_init_ring(&g_channels[i].ring, g_channels[i].backing_buffer,
                               (uint32_t)BHARAT_ARRAY_SIZE(g_channels[i].backing_buffer)) != URPC_SUCCESS) {
                return URPC_ERR_INVALID;
            }

            g_channels[i].in_use = 1U;
            g_channels[i].channel.sender_core_id = 0U;
            g_channels[i].channel.receiver_core_id = target_core;
            g_channels[i].channel.urpc_ring = &g_channels[i].ring;
            g_channels[i].channel.ring_size = g_channels[i].ring.capacity;

            *out_channel = g_channels[i].channel;
            return URPC_SUCCESS;
        }
    }

    return URPC_ERR_FULL;
}

int mk_send_message(mk_channel_t *channel, uint32_t msg_type, void *payload,
                    uint32_t size) {
    if (!channel || !channel->urpc_ring) {
        return URPC_ERR_INVALID;
    }

    if (size > 0U && !payload) {
        return URPC_ERR_INVALID;
    }

    urpc_msg_t msg = {0};
    msg.msg_type = msg_type;
    msg.payload_size = size;

    uint32_t words = size / sizeof(uint64_t);
    if ((size % sizeof(uint64_t)) != 0U) {
        words++;
    }
    if (words > MK_MAX_PAYLOAD_WORDS) {
        words = MK_MAX_PAYLOAD_WORDS;
        msg.payload_size = MK_MAX_PAYLOAD_WORDS * sizeof(uint64_t);
    }

    if (words > 0U) {
        uint64_t *src = (uint64_t *)payload;
        for (uint32_t i = 0; i < words; ++i) {
            msg.payload_data[i] = src[i];
        }
    }

    return urpc_send(channel->urpc_ring, &msg);
}

// Drains the incoming message queue, hands messages to IPC/dispatch path,
// and triggers scheduler wakeups if a waiter exists.
// This is the intended delivery mechanism for Phase 2/3, designed to be called
// from an interrupt-driven (IPI) context rather than relying purely on polling.
int mk_ipc_deliver_from_channel(mk_channel_t *channel) {
    if (!channel || !channel->urpc_ring) {
        return URPC_ERR_INVALID;
    }

    int processed = 0;
    urpc_msg_t msg;

    while (urpc_receive(channel->urpc_ring, &msg) == URPC_SUCCESS) {
        if (msg.msg_type == MK_MSG_TYPE_AI_SUGGESTION) {
            /* Scheduler control-plane hook placeholder. */
        } else {
            // Notify scheduler of IPC readiness / wake up waiting thread
            sched_notify_ipc_ready(channel->receiver_core_id, msg.msg_type);
        }
        ++processed;
    }

    return processed;
}

int mk_poll_messages(mk_channel_t *channel) {
    // Polling fallback path for environments without robust IPI receiver hooks.
    return mk_ipc_deliver_from_channel(channel);
}

int mk_msg_pool_init(mk_msg_pool_t* pool, mk_message_slot_t* slots, uint32_t capacity) {
    if (!pool || !slots || capacity == 0U) {
        return URPC_ERR_INVALID;
    }

    pool->slots = slots;
    pool->capacity = capacity;

    for (uint32_t i = 0; i < capacity; ++i) {
        pool->slots[i].in_use = 0U;
        pool->slots[i].msg.msg_type = 0U;
        pool->slots[i].msg.payload_size = 0U;
    }

    return URPC_SUCCESS;
}

urpc_msg_t* mk_msg_alloc(mk_msg_pool_t* pool) {
    if (!pool || !pool->slots || pool->capacity == 0U) {
        return NULL;
    }

    for (uint32_t i = 0; i < pool->capacity; ++i) {
        if (pool->slots[i].in_use == 0U) {
            pool->slots[i].in_use = 1U;
            return &pool->slots[i].msg;
        }
    }

    return NULL;
}

void mk_msg_free(mk_msg_pool_t* pool, urpc_msg_t* msg) {
    if (!pool || !pool->slots || !msg) {
        return;
    }

    for (uint32_t i = 0; i < pool->capacity; ++i) {
        if (&pool->slots[i].msg == msg) {
            pool->slots[i].in_use = 0U;
            pool->slots[i].msg.msg_type = 0U;
            pool->slots[i].msg.payload_size = 0U;
            return;
        }
    }
}
