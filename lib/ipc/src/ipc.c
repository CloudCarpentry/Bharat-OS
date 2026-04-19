#include <bharat/ipc/ipc.h>
#include <bharat/uapi/ipc/contract.h>
#include <stdint.h>

#include <bharat/syscalls.h>
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


#include <bharat/uapi/syscall_nr.h>
#include <bharat/uapi/syscall_args.h>
long bharat_syscall(long nr, long arg);

__attribute__((weak))
long bharat_ipc_transport_send(uint32_t send_cap, const void *payload, uint32_t len, uint64_t timeout_ticks) {
    bharat_sys_endpoint_send_args_t args = {
        .send_cap = send_cap,
        .payload_len = len,
        .payload_ptr = (uint64_t)(uintptr_t)payload,
        .timeout_ticks = timeout_ticks,
        .cap_to_send = 0U,
        .cap_send_rights = 0U,
        .reserved0 = 0,
    };
    return bharat_syscall(SYSCALL_ENDPOINT_SEND, (long)(uintptr_t)&args);
}

__attribute__((weak))
long bharat_ipc_transport_receive(uint32_t recv_cap, void *out_payload, uint32_t out_capacity, uint32_t *out_len, uint64_t timeout_ticks) {
    bharat_sys_endpoint_receive_args_t args = {
        .recv_cap = recv_cap,
        .out_payload_capacity = out_capacity,
        .out_payload_ptr = (uint64_t)(uintptr_t)out_payload,
        .out_len_ptr = (uint64_t)(uintptr_t)out_len,
        .timeout_ticks = timeout_ticks,
        .out_received_cap_ptr = 0,
    };
    return bharat_syscall(SYSCALL_ENDPOINT_RECEIVE, (long)(uintptr_t)&args);
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
    if (header->payload_size > 0U && payload == (void*)0) {
        return BHARAT_IPC_STATUS_ERR_DECODE;
    }

    msg.header = *header;
    if (header->payload_size > 0U) {
        uint8_t *dst = (uint8_t *)msg.payload;
        const uint8_t *src = (const uint8_t *)payload;
        for (uint32_t i = 0; i < header->payload_size; i++) {
            dst[i] = src[i];
        }
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
        uint8_t *dst = (uint8_t *)payload_buf;
        const uint8_t *src = (const uint8_t *)msg.payload;
        for (uint32_t i = 0; i < msg.header.payload_size; i++) {
            dst[i] = src[i];
        }
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
    int32_t st = bharat_ipc_send_ex(endpoint, req_header, req_payload, timeout_ticks);
    if (st != BHARAT_IPC_STATUS_OK) {
        return st;
    }

    st = bharat_ipc_recv_ex(endpoint, rep_header, rep_payload_buf, rep_max_size, timeout_ticks);
    if (st != BHARAT_IPC_STATUS_OK) {
        return st;
    }
    if (rep_header->message_id != req_header->message_id) {
        return BHARAT_IPC_STATUS_ERR_DECODE;
    }
    return rep_header->status;
}
