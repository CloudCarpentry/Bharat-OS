#include "../../include/advanced/multikernel.h"
#include "../../include/atomic.h"
#include "../../include/hal/hal.h"

#include <stddef.h>
#include <stdint.h>

#define URPC_SUCCESS 0
#define URPC_ERR_FULL -1
#define URPC_ERR_EMPTY -2
#define URPC_ERR_INVALID -3

#define MK_MAX_CHANNELS 16U
#define MK_MAX_PAYLOAD_WORDS 8U
#define MK_MSG_TYPE_AI_SUGGESTION 1U

typedef struct {
    urpc_msg_t backing_buffer[32];
    urpc_ring_t ring;
    mk_channel_t channel;
    uint8_t in_use;
} mk_channel_slot_t;

static mk_channel_slot_t g_channels[MK_MAX_CHANNELS];

void urpc_init_ring(urpc_ring_t* ring, urpc_msg_t* buffer_ptr, uint32_t ring_size) {
    if (!ring || !buffer_ptr || ring_size == 0U) {
        return;
    }

    ring->buffer = buffer_ptr;
    ring->capacity = ring_size;
    ring->head = 0U;
    ring->tail = 0U;
}

int urpc_send(urpc_ring_t* ring, urpc_msg_t* msg) {
    if (!ring || !msg || !ring->buffer || ring->capacity == 0U) {
        return URPC_ERR_INVALID;
    }

    uint32_t head = ring->head;
    uint32_t next_head = (head + 1U) % ring->capacity;

    if (next_head == ring->tail) {
        return URPC_ERR_FULL;
    }

    if (msg->payload_size <= sizeof(uint64_t)) {
        hal_send_ipi_payload(0U, msg->payload_data[0]);
    }

    ring->buffer[head] = *msg;
    smp_mb();
    ring->head = next_head;

    return URPC_SUCCESS;
}

int urpc_receive(urpc_ring_t* ring, urpc_msg_t* out_msg) {
    if (!ring || !out_msg || !ring->buffer || ring->capacity == 0U) {
        return URPC_ERR_INVALID;
    }

    uint32_t tail = ring->tail;
    if (tail == ring->head) {
        return URPC_ERR_EMPTY;
    }

    smp_mb();
    *out_msg = ring->buffer[tail];
    ring->tail = (tail + 1U) % ring->capacity;

    return URPC_SUCCESS;
}

int mk_establish_channel(uint32_t target_core, mk_channel_t *out_channel) {
    if (!out_channel) {
        return URPC_ERR_INVALID;
    }

    for (uint32_t i = 0; i < MK_MAX_CHANNELS; ++i) {
        if (g_channels[i].in_use == 0U) {
            g_channels[i].in_use = 1U;
            urpc_init_ring(&g_channels[i].ring, g_channels[i].backing_buffer,
                           (uint32_t)(sizeof(g_channels[i].backing_buffer) / sizeof(g_channels[i].backing_buffer[0])));

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
    if (!channel || !channel->urpc_ring || !payload) {
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

    uint64_t *src = (uint64_t *)payload;
    for (uint32_t i = 0; i < words; ++i) {
        msg.payload_data[i] = src[i];
    }

    return urpc_send(channel->urpc_ring, &msg);
}

int mk_poll_messages(mk_channel_t *channel) {
    if (!channel || !channel->urpc_ring) {
        return URPC_ERR_INVALID;
    }

    int processed = 0;
    urpc_msg_t msg;
    while (urpc_receive(channel->urpc_ring, &msg) == URPC_SUCCESS) {
        (void)msg;
        if (msg.msg_type == MK_MSG_TYPE_AI_SUGGESTION) {
            /* Scheduler control-plane hook placeholder. */
        }
        ++processed;
    }

    return processed;
}
