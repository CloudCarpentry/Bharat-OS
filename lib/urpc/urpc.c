#include "urpc.h"

int urpc_init_channel(urpc_channel_t* channel, void* shared_mem, uint32_t size_bytes) {
    if (!channel || !shared_mem || size_bytes < sizeof(urpc_msg_t)) {
        return URPC_ERR_INVALID;
    }

    uint32_t capacity = size_bytes / sizeof(urpc_msg_t);
    // Ensure capacity is a power of 2 for fast modulo
    if ((capacity & (capacity - 1)) != 0) {
        // Fallback or bit hack to get previous power of 2
        while ((capacity & (capacity - 1)) != 0) {
            capacity = capacity & (capacity - 1);
        }
    }

    if (capacity == 0) return URPC_ERR_INVALID;

    channel->head = 0;
    channel->tail = 0;
    channel->capacity = capacity;
    channel->mask = capacity - 1;
    channel->buffer = (urpc_msg_t*)shared_mem;
    return URPC_SUCCESS;
}

int urpc_send(urpc_channel_t* channel, const urpc_msg_t* msg) {
    if (!channel || !msg) return URPC_ERR_INVALID;

    // Check if buffer is full
    uint32_t head = __atomic_load_n(&channel->head, __ATOMIC_ACQUIRE);
    uint32_t tail = __atomic_load_n(&channel->tail, __ATOMIC_RELAXED);

    if (head - tail == channel->capacity) {
        return URPC_ERR_FULL;
    }

    uint32_t idx = head & channel->mask;

    // Copy payload (msg is fixed 64 bytes)
    for (int i = 0; i < sizeof(urpc_msg_t); i++) {
        ((uint8_t*)&channel->buffer[idx])[i] = ((const uint8_t*)msg)[i];
    }

    // Update head locklessly
    __atomic_store_n(&channel->head, head + 1, __ATOMIC_RELEASE);

    return URPC_SUCCESS;
}

int urpc_receive(urpc_channel_t* channel, urpc_msg_t* msg) {
    if (!channel || !msg) return URPC_ERR_INVALID;

    uint32_t tail = __atomic_load_n(&channel->tail, __ATOMIC_ACQUIRE);
    uint32_t head = __atomic_load_n(&channel->head, __ATOMIC_RELAXED);

    if (head == tail) {
        return URPC_ERR_EMPTY;
    }

    uint32_t idx = tail & channel->mask;

    for (int i = 0; i < sizeof(urpc_msg_t); i++) {
        ((uint8_t*)msg)[i] = ((const uint8_t*)&channel->buffer[idx])[i];
    }

    // Update tail locklessly
    __atomic_store_n(&channel->tail, tail + 1, __ATOMIC_RELEASE);

    return URPC_SUCCESS;
}
