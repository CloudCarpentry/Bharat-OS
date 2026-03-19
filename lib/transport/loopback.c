#include "bharat/msg/transport.h"

// Define internal helpers for freestanding environment.
static void *internal_memset(void *s, int c, size_t n) {
    unsigned char *p = s;
    while (n--) {
        *p++ = (unsigned char)c;
    }
    return s;
}

static void *internal_memcpy(void *dest, const void *src, size_t n) {
    unsigned char *d = dest;
    const unsigned char *s = src;
    while (n--) {
        *d++ = *s++;
    }
    return dest;
}

// Since we are building freestanding, mock malloc/free if they don't exist.
// This is just a loopback transport for unit testing.
static uint8_t static_lb_ctx_buf[2048];
static uint8_t static_lb_msg_buf[2048];
static int lb_alloc_in_use = 0;

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
    internal_memcpy(ctx->buffer, buf, len);
    ctx->stored_len = len;

    return BHARAT_MSG_OK;
}

static int lb_recv(bharat_transport_t* self, uint8_t* buf, size_t cap, size_t* out_len) {
    loopback_ctx_t* ctx = (loopback_ctx_t*)self->ctx;
    if (ctx->stored_len == 0) return BHARAT_MSG_ERR_TIMEOUT;

    if (cap < ctx->stored_len) {
        return BHARAT_MSG_ERR_BUFFER_OVERFLOW;
    }

    internal_memcpy(buf, ctx->buffer, ctx->stored_len);
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
        lb_alloc_in_use = 0;
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
    if (lb_alloc_in_use || max_mtu > sizeof(static_lb_msg_buf)) return -1;
    lb_alloc_in_use = 1;

    loopback_ctx_t* ctx = (loopback_ctx_t*)static_lb_ctx_buf;
    ctx->buffer = static_lb_msg_buf;
    ctx->stored_len = 0;
    ctx->mtu = max_mtu;

    t->ops = &lb_ops;
    t->ctx = ctx;
    t->local_id = 0; // Loopback usually resolves to ID 0
    return BHARAT_MSG_OK;
}
