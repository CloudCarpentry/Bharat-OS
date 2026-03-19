#include "bharat/msg/transport.h"
#include <stdlib.h>
#include <string.h>

// A very simple synchronous loopback ring buffer for unit testing.
// Real implementations will use URPC or Endpoint IPC.

typedef struct {
    uint8_t* buffer;
    size_t   stored_len;
    size_t   mtu;
} loopback_ctx_t;

static int lb_send(bharat_transport_t* self, const uint8_t* buf, size_t len) {
    loopback_ctx_t* ctx = (loopback_ctx_t*)self->ctx;
    if (len > ctx->mtu) return BHARAT_MSG_ERR_TOO_LARGE;

    // Synchronous overwrite of the single slot buffer (simple test mode)
    memcpy(ctx->buffer, buf, len);
    ctx->stored_len = len;

    return BHARAT_MSG_OK;
}

static int lb_recv(bharat_transport_t* self, uint8_t* buf, size_t cap, size_t* out_len) {
    loopback_ctx_t* ctx = (loopback_ctx_t*)self->ctx;
    if (ctx->stored_len == 0) return BHARAT_MSG_ERR_TIMEOUT;

    if (cap < ctx->stored_len) {
        return BHARAT_MSG_ERR_BUFFER_OVERFLOW;
    }

    memcpy(buf, ctx->buffer, ctx->stored_len);
    *out_len = ctx->stored_len;

    // Clear out
    ctx->stored_len = 0;

    return BHARAT_MSG_OK;
}

static uint32_t lb_get_caps(bharat_transport_t* self) {
    (void)self;
    // Loopback allows everything within a local context.
    return BHARAT_TRANSPORT_CAP_RELIABLE | BHARAT_TRANSPORT_CAP_ORDERED | BHARAT_TRANSPORT_CAP_LOCAL_ONLY;
}

static size_t lb_get_mtu(bharat_transport_t* self) {
    loopback_ctx_t* ctx = (loopback_ctx_t*)self->ctx;
    return ctx->mtu;
}

static int lb_poll(bharat_transport_t* self, int timeout_ms) {
    (void)timeout_ms;
    loopback_ctx_t* ctx = (loopback_ctx_t*)self->ctx;
    return (ctx->stored_len > 0) ? 1 : 0;
}

static int lb_ack(bharat_transport_t* self, uint64_t request_id) {
    (void)self;
    (void)request_id;
    return BHARAT_MSG_OK;
}

static int lb_close(bharat_transport_t* self) {
    if (self->ctx) {
        loopback_ctx_t* ctx = (loopback_ctx_t*)self->ctx;
        free(ctx->buffer);
        free(ctx);
        self->ctx = NULL;
    }
    return BHARAT_MSG_OK;
}

static const bharat_transport_ops_t lb_ops = {
    .send     = lb_send,
    .recv     = lb_recv,
    .get_caps = lb_get_caps,
    .get_mtu  = lb_get_mtu,
    .poll     = lb_poll,
    .ack      = lb_ack,
    .close    = lb_close,
};

int bharat_transport_loopback_create(bharat_transport_t* t, size_t max_mtu) {
    loopback_ctx_t* ctx = (loopback_ctx_t*)malloc(sizeof(loopback_ctx_t));
    if (!ctx) return -1;

    ctx->buffer = (uint8_t*)malloc(max_mtu);
    if (!ctx->buffer) {
        free(ctx);
        return -1;
    }

    ctx->stored_len = 0;
    ctx->mtu = max_mtu;

    t->ops = &lb_ops;
    t->ctx = ctx;
    t->local_id = 0; // Loopback usually resolves to ID 0
    return BHARAT_MSG_OK;
}
