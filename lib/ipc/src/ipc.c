#include <bharat/ipc/ipc.h>
#include <bharat/uapi/ipc/contract.h>
#include <ipc_user.h>
#include <stddef.h>
#include <stdint.h>

void *memcpy(void *dest, const void *src, size_t n);

#define BHARAT_IPC_ENDPOINT_PAYLOAD_MAX 128u

typedef struct {
    bharat_ipc_msg_header_t header;
    uint8_t payload[BHARAT_IPC_ENDPOINT_PAYLOAD_MAX - sizeof(bharat_ipc_msg_header_t)];
} bharat_ipc_wire_msg_t;

_Static_assert(sizeof(bharat_ipc_wire_msg_t) <= BHARAT_IPC_ENDPOINT_PAYLOAD_MAX, "IPC wire message exceeds endpoint payload size");

static int32_t ipc_check_header(const bharat_ipc_msg_header_t *header) {
    if (!header) {
        return BHARAT_IPC_STATUS_ERR_DECODE;
    }
    if (header->header_version != BHARAT_IPC_HEADER_VERSION_V1) {
        return BHARAT_IPC_STATUS_ERR_VERSION;
    }
    if (header->payload_size > sizeof(((bharat_ipc_wire_msg_t *)0)->payload)) {
        return BHARAT_IPC_STATUS_ERR_LENGTH;
    }
    return BHARAT_IPC_STATUS_OK;
}

__attribute__((weak))
long bharat_ipc_transport_send(uint32_t send_cap, const void *payload, uint32_t len, uint64_t timeout_ticks) {
    return ipc_endpoint_send(send_cap, payload, len, timeout_ticks, 0U, 0U);
}

__attribute__((weak))
long bharat_ipc_transport_receive(uint32_t recv_cap, void *out_payload, uint32_t out_capacity, uint32_t *out_len, uint64_t timeout_ticks) {
    return ipc_endpoint_receive(recv_cap, out_payload, out_capacity, out_len, timeout_ticks, NULL);
}

static int32_t ipc_status_from_kernel(long status) {
    switch (status) {
        case 0:  return BHARAT_IPC_STATUS_OK;
        case -1: return BHARAT_IPC_STATUS_ERR_INVALID;
        case -2: return BHARAT_IPC_STATUS_ERR_TRUNCATED;
        case -3: return BHARAT_IPC_STATUS_ERR_BUSY;
        case -4: return BHARAT_IPC_STATUS_ERR_PERM;
        case -7: return BHARAT_IPC_STATUS_ERR_ENDPOINT;
        case -8: return BHARAT_IPC_STATUS_ERR_TIMEOUT;
        default: return BHARAT_IPC_STATUS_ERR_INTERNAL;
    }
}

int32_t bharat_ipc_send(bharat_ipc_endpoint_t endpoint, const bharat_ipc_msg_header_t *header, const void *payload) {
    return bharat_ipc_send_ex(endpoint, header, payload, UINT64_MAX);
}

int32_t bharat_ipc_recv(bharat_ipc_endpoint_t endpoint, bharat_ipc_msg_header_t *header, void *payload_buf, uint32_t max_size) {
    return bharat_ipc_recv_ex(endpoint, header, payload_buf, max_size, UINT64_MAX);
}

int32_t bharat_ipc_call(bharat_ipc_endpoint_t endpoint, const bharat_ipc_msg_header_t *req_header, const void *req_payload,
                        bharat_ipc_msg_header_t *rep_header, void *rep_payload_buf, uint32_t rep_max_size) {
    return bharat_ipc_call_ex(endpoint, req_header, req_payload, rep_header, rep_payload_buf, rep_max_size, UINT64_MAX);
}

int32_t bharat_ipc_send_ex(bharat_ipc_endpoint_t endpoint,
                           const bharat_ipc_msg_header_t *header,
                           const void *payload,
                           uint64_t timeout_ticks) {
    bharat_ipc_wire_msg_t msg = {0};
    int32_t valid = ipc_check_header(header);
    if (valid != BHARAT_IPC_STATUS_OK) {
        return valid;
    }
    if (header->payload_size > 0U && payload == NULL) {
        return BHARAT_IPC_STATUS_ERR_DECODE;
    }

    msg.header = *header;
    if (header->payload_size > 0U) {
        memcpy(msg.payload, payload, header->payload_size);
    }

    long st = bharat_ipc_transport_send(endpoint, &msg, (uint32_t)(sizeof(msg.header) + header->payload_size), timeout_ticks);
    return ipc_status_from_kernel(st);
}

int32_t bharat_ipc_recv_ex(bharat_ipc_endpoint_t endpoint,
                           bharat_ipc_msg_header_t *header,
                           void *payload_buf,
                           uint32_t max_size,
                           uint64_t timeout_ticks) {
    bharat_ipc_wire_msg_t msg = {0};
    uint32_t recv_len = 0;
    long st;

    if (!header || (!payload_buf && max_size > 0U)) {
        return BHARAT_IPC_STATUS_ERR_DECODE;
    }

    st = bharat_ipc_transport_receive(endpoint, &msg, sizeof(msg), &recv_len, timeout_ticks);
    if (st != 0) {
        return ipc_status_from_kernel(st);
    }
    if (recv_len < sizeof(bharat_ipc_msg_header_t)) {
        return BHARAT_IPC_STATUS_ERR_DECODE;
    }
    if (ipc_check_header(&msg.header) != BHARAT_IPC_STATUS_OK) {
        return BHARAT_IPC_STATUS_ERR_VERSION;
    }
    if ((sizeof(msg.header) + msg.header.payload_size) > recv_len || msg.header.payload_size > max_size) {
        return BHARAT_IPC_STATUS_ERR_TRUNCATED;
    }

    *header = msg.header;
    if (msg.header.payload_size > 0U) {
        memcpy(payload_buf, msg.payload, msg.header.payload_size);
    }
    return BHARAT_IPC_STATUS_OK;
}

int32_t bharat_ipc_call_ex(bharat_ipc_endpoint_t endpoint,
                           const bharat_ipc_msg_header_t *req_header,
                           const void *req_payload,
                           bharat_ipc_msg_header_t *rep_header,
                           void *rep_payload_buf,
                           uint32_t rep_max_size,
                           uint64_t timeout_ticks) {
    // 1. Create a temporary endpoint for the reply.
    uint32_t reply_send_cap;
    uint32_t reply_recv_cap;
    long create_st = ipc_endpoint_create(&reply_send_cap, &reply_recv_cap);
    if (create_st != 0) {
        return ipc_status_from_kernel(create_st);
    }

    // 2. Clone the request header to inject the reply capability.
    bharat_ipc_msg_header_t actual_req_header = *req_header;
    actual_req_header.reply_endpoint = reply_send_cap;

    // 3. Send the request (transmitting the reply_send_cap if applicable,
    //    though right now we rely on the header field for the destination ID).
    int32_t st = bharat_ipc_send_ex(endpoint, &actual_req_header, req_payload, timeout_ticks);
    if (st != BHARAT_IPC_STATUS_OK) {
        // TODO: Need an ipc_endpoint_destroy call here when available
        return st;
    }

    // 4. Wait for the response on the new receive capability.
    st = bharat_ipc_recv_ex(reply_recv_cap, rep_header, rep_payload_buf, rep_max_size, timeout_ticks);

    // TODO: Clean up the temporary endpoint caps here

    if (st != BHARAT_IPC_STATUS_OK) {
        return st;
    }
    if (rep_header->message_id != actual_req_header.message_id) {
        return BHARAT_IPC_STATUS_ERR_DECODE;
    }
    return rep_header->status;
}
