#ifndef BHARAT_URPC_H
#define BHARAT_URPC_H

#include <stdint.h>
#include <stdbool.h>
#include "urpc/urpc_bootstrap.h"

// Define standard URPC message types
typedef enum {
    URPC_MSG_NONE            = 0,
    URPC_BIND_REQ            = 1,
    URPC_BIND_ACK            = 2,
    URPC_CLOSE               = 3,
    URPC_CAP_REVOKE          = 4,
    URPC_CAP_REVOKE_ACK      = 5,
    URPC_TLB_INVAL           = 6,
    URPC_TLB_INVAL_ACK       = 7
} urpc_msg_type_t;

// A generic URPC message packet structure fitting in the uint64_t buffer
// Top 8 bits = message type
// Lower 56 bits = payload

#define URPC_MSG_TYPE_SHIFT 56
#define URPC_MSG_TYPE_MASK  0xFFULL
#define URPC_MSG_PAYLOAD_MASK ((1ULL << 56) - 1ULL)

static inline uint64_t urpc_pack_msg(urpc_msg_type_t type, uint64_t payload) {
    return (((uint64_t)type & URPC_MSG_TYPE_MASK) << URPC_MSG_TYPE_SHIFT) | (payload & URPC_MSG_PAYLOAD_MASK);
}

static inline void urpc_unpack_msg(uint64_t raw_msg, urpc_msg_type_t* type, uint64_t* payload) {
    if (type) *type = (urpc_msg_type_t)((raw_msg >> URPC_MSG_TYPE_SHIFT) & URPC_MSG_TYPE_MASK);
    if (payload) *payload = (raw_msg & URPC_MSG_PAYLOAD_MASK);
}

// Channel state tracking
typedef enum {
    URPC_CHANNEL_CLOSED,
    URPC_CHANNEL_BINDING,
    URPC_CHANNEL_BOUND,
    URPC_CHANNEL_ERROR
} urpc_channel_state_t;

int urpc_channel_bind(uint32_t target_core);
int urpc_channel_accept(uint32_t source_core);
int urpc_channel_close(uint32_t target_core);
urpc_channel_state_t urpc_channel_get_state(uint32_t target_core);

#endif // BHARAT_URPC_H
