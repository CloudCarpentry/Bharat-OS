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
long bharat_ipc_transport_receive(uint32_t recv_cap, void *out_payload, uint32_t out_capacity, uint32_t *out_len, uint64_t timeout_ticks) {
    return ipc_endpoint_receive(recv_cap, out_payload, out_capacity, out_len, timeout_ticks, NULL);
}

__attribute__((weak))
long bharat_ipc_transport_receive_envelope(uint32_t recv_cap, void *out_payload, uint32_t out_capacity, uint32_t *out_len, uint64_t timeout_ticks, bh_ipc_receive_envelope_v1_t *out_envelope) {
    return ipc_endpoint_receive_envelope(recv_cap, out_payload, out_capacity, out_len, timeout_ticks, out_envelope);
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

int32_t bharat_ipc_send(bharat_ipc_endpoint_t endpoint, const bharat_ipc_msg_header_t *header, const void *payload) {
    return bharat_ipc_send_ex(endpoint, header, payload, UINT64_MAX);
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

    /* V1 has neither authenticated reply provenance nor endpoint/cap release.
     * Allocating a fresh reply endpoint would leak on every outcome, including
     * send failure and timeout. Do not start an RPC until both contracts exist.
     * In particular, do not subtract TIME_GET nanoseconds from scheduler ticks.
     * Keep outputs unchanged and allocate no kernel resources on this path. */
    (void)endpoint;
    (void)timeout_ticks;
    return BHARAT_IPC_STATUS_ERR_UNSUPPORTED;
}
