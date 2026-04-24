#include "bharat/msg/transport.h"
#include "bharat/msg/wire.h"
#include "urpc/urpc.h"

#define URPC_MAX_MSG 2048

static int urpc_send_msg(bharat_transport_t* t,
                         const uint8_t* data,
                         size_t len)
{
    if (len > URPC_MAX_MSG) {
        return BHARAT_ERR_MSG_TOO_LARGE;
    }

    // In a real implementation this would map to actual urpc rings
    // For this prototype, we'll wrap a generic urpc_send if the context holds a channel
    urpc_channel_t* channel = (urpc_channel_t*)t->ctx;

    if (!channel) return BHARAT_ERR_INVALID_PARAM;

    // Convert to urpc_msg_t format
    urpc_msg_t umsg = {0};
    umsg.length = (uint32_t)len;
    // We copy what we can fit into the payload block for now
    size_t copy_len = len < sizeof(umsg.payload) ? len : sizeof(umsg.payload);
    for (size_t i = 0; i < copy_len; i++) {
        umsg.payload[i] = data[i];
    }

    return urpc_send(channel, &umsg);
}

static int urpc_recv_msg(bharat_transport_t* t,
                         uint8_t* buffer,
                         size_t max_len,
                         size_t* out_len)
{
    urpc_channel_t* channel = (urpc_channel_t*)t->ctx;
    if (!channel) return BHARAT_ERR_INVALID_PARAM;

    urpc_msg_t umsg;
    int ret = urpc_receive(channel, &umsg);
    if (ret != URPC_SUCCESS) {
        return ret; // e.g. URPC_ERR_EMPTY
    }

    *out_len = umsg.length;

    if (*out_len > max_len) {
        return BHARAT_ERR_MSG_TOO_LARGE;
    }

    size_t copy_len = *out_len < sizeof(umsg.payload) ? *out_len : sizeof(umsg.payload);
    for (size_t i = 0; i < copy_len; i++) {
        buffer[i] = umsg.payload[i];
    }

    // Validate header before passing upward
    bharat_msg_header_t hdr;
    if (bharat_msg_parse_header(buffer, *out_len, &hdr) != 0) {
        return BHARAT_ERR_MALFORMED;
    }

    return 0;
}

static uint32_t urpc_get_caps(struct bharat_transport* self)
{
    return BHARAT_TRANSPORT_CAP_RELIABLE |
           BHARAT_TRANSPORT_CAP_ORDERED |
           BHARAT_TRANSPORT_CAP_LOCAL_ONLY;
}

static size_t urpc_get_mtu(struct bharat_transport* self)
{
    return 56; // payload size of urpc_msg_t
}

bharat_transport_ops_t urpc_ops = {
    .send = urpc_send_msg,
    .recv = urpc_recv_msg,
    .get_caps = urpc_get_caps,
    .get_mtu = urpc_get_mtu,
    .poll = NULL,
    .ack = NULL,
    .close = NULL
};
