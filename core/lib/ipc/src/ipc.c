#include <bharat/uapi/capability/rights.h>
#include <bharat/cap/cap.h>
#include <bharat/ipc/ipc.h>
#include <bharat/uapi/ipc/contract.h>
#include <bharat/uapi/syscall/bh_syscall_status.h>
#include <ipc_user.h>
#include <lib/base/string.h>
#include <stdint.h>
#include <stddef.h>

#define memcpy __builtin_memcpy

/* Aligned with BH_CAP_RIGHT_ENDPOINT_SEND in <bharat/uapi/capability/rights.h> */
#ifndef BH_KERNEL_CAP_RIGHT_ENDPOINT_SEND
#define BH_KERNEL_CAP_RIGHT_ENDPOINT_SEND BH_CAP_RIGHT_ENDPOINT_SEND
#endif

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
    if ((header->flags & ~BHARAT_IPC_FLAGS_ALL_V1) != 0U) {
        return BHARAT_IPC_STATUS_ERR_FLAGS;
    }
    return BHARAT_IPC_STATUS_OK;
}

__attribute__((weak))
long bharat_ipc_transport_send(uint32_t send_cap, const void *payload, uint32_t len, uint64_t timeout_ticks) {
    return ipc_endpoint_send(send_cap, payload, len, timeout_ticks, 0U, 0U);
}

__attribute__((weak))
long bharat_ipc_transport_send_caps(uint32_t send_cap,
                                    const void *payload,
                                    uint32_t len,
                                    uint64_t timeout_ticks,
                                    uint32_t cap_to_send,
                                    uint64_t cap_rights,
                                    uint32_t reply_cap_to_send,
                                    uint64_t reply_cap_rights) {
    if (cap_to_send == 0U && reply_cap_to_send == 0U) {
        return bharat_ipc_transport_send(send_cap, payload, len, timeout_ticks);
    }
    return ipc_endpoint_send_ex(send_cap, payload, len, timeout_ticks,
                                cap_to_send, cap_rights,
                                reply_cap_to_send, reply_cap_rights);
}

__attribute__((weak))
long bharat_ipc_transport_receive(uint32_t recv_cap, void *out_payload, uint32_t out_capacity, uint32_t *out_len, uint64_t timeout_ticks) {
    return ipc_endpoint_receive(recv_cap, out_payload, out_capacity, out_len, timeout_ticks, NULL);
}

__attribute__((weak))
long bharat_ipc_transport_receive_envelope(uint32_t recv_cap, void *out_payload, uint32_t out_capacity, uint32_t *out_len, uint64_t timeout_ticks, bh_ipc_receive_envelope_v1_t *out_envelope) {
    return ipc_endpoint_receive_envelope(recv_cap, out_payload, out_capacity, out_len, timeout_ticks, out_envelope);
}

__attribute__((weak))
long bharat_ipc_transport_create_endpoint(uint32_t *out_send_cap, uint32_t *out_recv_cap) {
    return ipc_endpoint_create(out_send_cap, out_recv_cap);
}

__attribute__((weak))
long bharat_ipc_transport_close_endpoint(uint32_t cap) {
    return ipc_endpoint_close(cap);
}

static int32_t ipc_status_from_kernel(long status) {
    /* Endpoint syscalls return the native bh_status_t namespace, after the
     * kernel translates its internal IPC result. Never decode raw IPC enums. */
    switch (status) {
        case BH_OK: return BHARAT_IPC_STATUS_OK;
        case BH_ERR_INVALID_ARGUMENT: return BHARAT_IPC_STATUS_ERR_INVALID;
        case BH_ERR_INVALID_SYSCALL:
        case BH_ERR_NOT_SUPPORTED: return BHARAT_IPC_STATUS_ERR_UNSUPPORTED;
        case BH_ERR_ACCESS_DENIED:
        case BH_ERR_BAD_CAPABILITY:
        case BH_ERR_STALE_CAPABILITY:
        case BH_ERR_WRONG_TYPE:
        case BH_ERR_INSUFFICIENT_RIGHTS: return BHARAT_IPC_STATUS_ERR_PERM;
        case BH_ERR_NO_MEMORY:
        case BH_ERR_RESOURCE_EXHAUSTED: return BHARAT_STATUS_ERR_NO_MEMORY;
        case BH_ERR_TRY_AGAIN:
        case BH_ERR_BUFFER_FULL: return BHARAT_IPC_STATUS_ERR_BUSY;
        case BH_ERR_BAD_STATE: return BHARAT_STATUS_ERR_BAD_STATE;
        case BH_ERR_TIMEOUT: return BHARAT_IPC_STATUS_ERR_TIMEOUT;
        case BH_ERR_OVERFLOW: return BHARAT_IPC_STATUS_ERR_LENGTH;
        case BH_ERR_CHANNEL_CLOSED:
        case BH_ERR_NOT_FOUND: return BHARAT_IPC_STATUS_ERR_ENDPOINT;
        case BH_ERR_FAULT:
        case BH_ERR_INTERRUPTED:
        case BH_ERR_INTERNAL:
        default: return BHARAT_IPC_STATUS_ERR_INTERNAL;
    }
}

int32_t bharat_ipc_endpoint_close(bharat_ipc_endpoint_t endpoint) {
    if (endpoint == BHARAT_CAP_INVALID_HANDLE || endpoint > UINT32_MAX) {
        return BHARAT_IPC_STATUS_ERR_INVALID;
    }
    return ipc_status_from_kernel(bharat_ipc_transport_close_endpoint((uint32_t)endpoint));
}

int32_t bharat_ipc_send(bharat_ipc_endpoint_t endpoint, const bharat_ipc_msg_header_t *header, const void *payload) {
    return bharat_ipc_send_ex(endpoint, header, payload, UINT64_MAX);
}

int32_t bharat_ipc_send_cap(bharat_ipc_endpoint_t endpoint,
                            const bharat_ipc_msg_header_t *header,
                            const void *payload,
                            bharat_cap_handle_t cap_to_send,
                            uint64_t cap_send_rights) {
    return bharat_ipc_send_cap_ex(endpoint, header, payload, cap_to_send, cap_send_rights, UINT64_MAX);
}

int32_t bharat_ipc_send_cap_ex(bharat_ipc_endpoint_t endpoint,
                               const bharat_ipc_msg_header_t *header,
                               const void *payload,
                               bharat_cap_handle_t cap_to_send,
                               uint64_t cap_send_rights,
                               uint64_t timeout_ticks) {
    bharat_ipc_wire_msg_t msg = {0};
    int32_t valid = ipc_check_header(header);
    if (valid != BHARAT_IPC_STATUS_OK) {
        return valid;
    }
    if (header->payload_size > 0U && payload == NULL) {
        return BHARAT_IPC_STATUS_ERR_DECODE;
    }
    if (endpoint == BHARAT_CAP_INVALID_HANDLE || endpoint > UINT32_MAX || cap_to_send > UINT32_MAX) {
        return BHARAT_IPC_STATUS_ERR_INVALID;
    }

    msg.header = *header;
    msg.header.capability_transfer = 0U;
    msg.header.reply_endpoint = 0U;
    msg.header.flags &= ~BHARAT_IPC_FLAG_CAP_TRANSFER;
    if (header->payload_size > 0U) {
        memcpy(msg.payload, payload, header->payload_size);
    }

    long st = bharat_ipc_transport_send_caps((uint32_t)endpoint,
                                             &msg,
                                             (uint32_t)(sizeof(msg.header) + header->payload_size),
                                             timeout_ticks,
                                             (uint32_t)cap_to_send,
                                             cap_send_rights,
                                             0U,
                                             0U);
    return ipc_status_from_kernel(st);
}

int32_t bharat_ipc_recv(bharat_ipc_endpoint_t endpoint, bharat_ipc_msg_header_t *header, void *payload_buf, uint32_t max_size) {
    return bharat_ipc_recv_ex(endpoint, header, payload_buf, max_size, UINT64_MAX);
}

int32_t bharat_ipc_recv_envelope(bharat_ipc_endpoint_t endpoint,
                                 bharat_ipc_msg_header_t *header,
                                 void *payload_buf,
                                 uint32_t max_size,
                                 bh_ipc_receive_envelope_v1_t *out_envelope) {
    return bharat_ipc_recv_envelope_ex(endpoint, header, payload_buf, max_size, UINT64_MAX, out_envelope);
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

    /* This API has no capability-transfer argument. Serialized handles do not
     * grant authority and must not advertise a transfer or a reply channel. */
    if ((header->flags & BHARAT_IPC_FLAG_CAP_TRANSFER) != 0U ||
        header->capability_transfer != 0U || header->reply_endpoint != 0U) {
        return BHARAT_IPC_STATUS_ERR_UNSUPPORTED;
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

    /* V1 cannot attest the transferred object's type or role. Decline the cap
     * at the kernel boundary, which owns revocation on success and copy faults.
     * Never receive an unusable handle that this ABI cannot release. */
    st = ipc_endpoint_receive(endpoint, &msg, sizeof(msg), &recv_len, timeout_ticks, NULL);
    if (st != 0) {
        return ipc_status_from_kernel(st);
    }
    if (recv_len < sizeof(bharat_ipc_msg_header_t)) {
        return BHARAT_IPC_STATUS_ERR_DECODE;
    }
    int32_t header_st = ipc_check_header(&msg.header);
    if (header_st != BHARAT_IPC_STATUS_OK) {
        return header_st;
    }
    if ((sizeof(msg.header) + msg.header.payload_size) != recv_len) {
        return BHARAT_IPC_STATUS_ERR_LENGTH;
    }
    if (msg.header.payload_size > max_size) {
        return BHARAT_IPC_STATUS_ERR_TRUNCATED;
    }

    if ((msg.header.flags & BHARAT_IPC_FLAG_CAP_TRANSFER) != 0U) {
        return BHARAT_IPC_STATUS_ERR_UNSUPPORTED;
    }

    /* These wire integers are attacker controlled, even if a cap was sent.
     * Only a future versioned kernel envelope may supply local authority. */
    msg.header.capability_transfer = 0U;
    msg.header.reply_endpoint = 0U;
    *header = msg.header;

    if (msg.header.payload_size > 0U) {
        memcpy(payload_buf, msg.payload, msg.header.payload_size);
    }
    return BHARAT_IPC_STATUS_OK;
}

int32_t bharat_ipc_recv_envelope_ex(bharat_ipc_endpoint_t endpoint,
                                    bharat_ipc_msg_header_t *header,
                                    void *payload_buf,
                                    uint32_t max_size,
                                    uint64_t timeout_ticks,
                                    bh_ipc_receive_envelope_v1_t *out_envelope) {
    bharat_ipc_wire_msg_t msg = {0};
    uint32_t recv_len = 0;
    long st;

    if (!header || (!payload_buf && max_size > 0U) || !out_envelope) {
        return BHARAT_IPC_STATUS_ERR_DECODE;
    }

    st = bharat_ipc_transport_receive_envelope(endpoint, &msg, sizeof(msg), &recv_len, timeout_ticks, out_envelope);
    if (st != 0) {
        return ipc_status_from_kernel(st);
    }
    if (recv_len < sizeof(bharat_ipc_msg_header_t)) {
        return BHARAT_IPC_STATUS_ERR_DECODE;
    }
    int32_t header_st = ipc_check_header(&msg.header);
    if (header_st != BHARAT_IPC_STATUS_OK) {
        return header_st;
    }
    if ((sizeof(msg.header) + msg.header.payload_size) != recv_len) {
        return BHARAT_IPC_STATUS_ERR_LENGTH;
    }
    if (msg.header.payload_size > max_size) {
        return BHARAT_IPC_STATUS_ERR_TRUNCATED;
    }

    /* Wire integers are untrusted payload metadata. Real transferred handles
     * and provenance reside in out_envelope. */
    msg.header.capability_transfer = 0U;
    msg.header.reply_endpoint = 0U;
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
    if (!req_header || !rep_header || (rep_max_size > 0U && rep_payload_buf == NULL)) {
        return BHARAT_IPC_STATUS_ERR_DECODE;
    }

    int32_t valid = ipc_check_header(req_header);
    if (valid != BHARAT_IPC_STATUS_OK) {
        return valid;
    }
    if (req_header->payload_size > 0U && req_payload == NULL) {
        return BHARAT_IPC_STATUS_ERR_DECODE;
    }
    if (endpoint == BHARAT_CAP_INVALID_HANDLE || endpoint > UINT32_MAX ||
        req_header->capability_transfer > UINT32_MAX) {
        return BHARAT_IPC_STATUS_ERR_INVALID;
    }
    /* Never allow caller-supplied reply_endpoint on the wire; the RPC transport
     * creates and owns the ephemeral reply endpoint. */
    if (req_header->reply_endpoint != 0U) {
        return BHARAT_IPC_STATUS_ERR_UNSUPPORTED;
    }

    uint32_t reply_send_cap = 0U;
    uint32_t reply_recv_cap = 0U;
    long create_st = bharat_ipc_transport_create_endpoint(&reply_send_cap, &reply_recv_cap);
    if (create_st != 0 || reply_send_cap == 0U || reply_recv_cap == 0U) {
        return ipc_status_from_kernel(create_st != 0 ? create_st : BH_ERR_NO_MEMORY);
    }

    bharat_ipc_wire_msg_t req_msg = {0};
    req_msg.header = *req_header;
    req_msg.header.capability_transfer = 0U;
    req_msg.header.reply_endpoint = 0U;
    req_msg.header.flags &= ~BHARAT_IPC_FLAG_CAP_TRANSFER;
    if (req_header->payload_size > 0U) {
        memcpy(req_msg.payload, req_payload, req_header->payload_size);
    }

    uint32_t auth_cap = (uint32_t)req_header->capability_transfer;
    uint64_t auth_rights = auth_cap != 0U ? BH_KERNEL_CAP_RIGHT_ENDPOINT_SEND : 0U;
    long send_st = bharat_ipc_transport_send_caps((uint32_t)endpoint,
                                                  &req_msg,
                                                  (uint32_t)(sizeof(req_msg.header) + req_header->payload_size),
                                                  timeout_ticks,
                                                  auth_cap,
                                                  auth_rights,
                                                  reply_send_cap,
                                                  BH_KERNEL_CAP_RIGHT_ENDPOINT_SEND);
    if (send_st != 0) {
        (void)bharat_ipc_transport_close_endpoint(reply_send_cap);
        (void)bharat_ipc_transport_close_endpoint(reply_recv_cap);
        return ipc_status_from_kernel(send_st);
    }

    bharat_ipc_msg_header_t tmp_rep_hdr = {0};
    uint8_t tmp_rep_payload[BHARAT_IPC_ENDPOINT_PAYLOAD_MAX - sizeof(bharat_ipc_msg_header_t)] = {0};
    uint32_t bound_max = rep_max_size < sizeof(tmp_rep_payload) ? rep_max_size : (uint32_t)sizeof(tmp_rep_payload);
    bh_ipc_receive_envelope_v1_t rep_env = {0};

    int32_t recv_st = bharat_ipc_recv_envelope_ex(reply_recv_cap,
                                                  &tmp_rep_hdr,
                                                  bound_max > 0U ? tmp_rep_payload : NULL,
                                                  bound_max,
                                                  timeout_ticks,
                                                  &rep_env);

    /* Always close and revoke our local reply capabilities before returning.
     * Revoking reply_send_cap also revokes any unclosed server-side delegation. */
    (void)bharat_ipc_transport_close_endpoint(reply_send_cap);
    (void)bharat_ipc_transport_close_endpoint(reply_recv_cap);

    if (recv_st != BHARAT_IPC_STATUS_OK) {
        if (rep_env.authority_cap != 0U) {
            (void)bharat_ipc_transport_close_endpoint(rep_env.authority_cap);
        }
        return recv_st;
    }

    uint32_t returned_cap = rep_env.authority_cap != 0U ? rep_env.authority_cap : rep_env.reply_send_cap;
    if (req_header->message_id != 0U && tmp_rep_hdr.message_id != req_header->message_id) {
        if (returned_cap != 0U) {
            (void)bharat_ipc_transport_close_endpoint(returned_cap);
        }
        return BHARAT_IPC_STATUS_ERR_DECODE;
    }

    tmp_rep_hdr.capability_transfer = returned_cap;
    *rep_header = tmp_rep_hdr;
    if (tmp_rep_hdr.payload_size > 0U && rep_payload_buf != NULL) {
        memcpy(rep_payload_buf, tmp_rep_payload, tmp_rep_hdr.payload_size);
    }
    return BHARAT_IPC_STATUS_OK;
}
