#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "bharat/ipc/ipc.h"
#include <bharat/uapi/syscall_args.h>
#include <bharat/uapi/syscall_nr.h>
#include <bharat/uapi/syscall/bh_syscall_status.h>

// Mock state
typedef struct {
    uint8_t bytes[128];
    uint32_t len;
} queued_msg_t;

static queued_msg_t g_rx_queue[32];
static uint32_t g_rx_head;
static uint32_t g_rx_tail;
static long g_next_send_status;
static long g_next_recv_status;
static uint32_t g_create_count;
static uint32_t g_close_count;
static uint32_t g_send_count;
static uint32_t g_recv_count;
static uint32_t g_discarded_caps;
static uint32_t g_pending_cap;
static uint32_t g_last_sent_cap;
static uint32_t g_last_sent_reply_cap;
static uint64_t g_last_sent_reply_rights;
static uint64_t g_last_send_timeout;
static uint64_t g_last_recv_timeout;

static queued_msg_t g_last_sent;

long bharat_ipc_transport_send(uint32_t send_cap, const void *payload, uint32_t len, uint64_t timeout_ticks) {
    (void)send_cap;
    g_send_count++;
    g_last_send_timeout = timeout_ticks;
    if (g_next_send_status != 0) {
        long st = g_next_send_status;
        g_next_send_status = 0;
        return st;
    }
    memcpy(g_last_sent.bytes, payload, len);
    g_last_sent.len = len;
    return 0;
}

long bharat_ipc_transport_receive(uint32_t recv_cap, void *out_payload, uint32_t out_capacity, uint32_t *out_len, uint64_t timeout_ticks) {
    (void)recv_cap;
    g_recv_count++;
    g_last_recv_timeout = timeout_ticks;
    if (g_next_recv_status != 0) {
        long st = g_next_recv_status;
        g_next_recv_status = 0;
        return st;
    }
    if (g_rx_head == g_rx_tail) {
        return BH_ERR_TIMEOUT;
    }
    uint32_t idx = g_rx_head % 32;
    if (g_rx_queue[idx].len > out_capacity) {
        return BH_ERR_OVERFLOW;
    }
    memcpy(out_payload, g_rx_queue[idx].bytes, g_rx_queue[idx].len);
    *out_len = g_rx_queue[idx].len;
    g_rx_head++;
    return 0;
}

static void queue_rx(const void* data, uint32_t len) {
    uint32_t idx = g_rx_tail % 32;
    memcpy(g_rx_queue[idx].bytes, data, len);
    g_rx_queue[idx].len = len;
    g_rx_tail++;
}

long bh_syscall(long sysno, long arg1, long arg2, long arg3, long arg4, long arg5, long arg6) {
    (void)arg3; (void)arg4; (void)arg5; (void)arg6;
    if (sysno == SYSCALL_CAPABILITY_INVOKE) {
        const bharat_sys_cap_invoke_args_t *args = (const bharat_sys_cap_invoke_args_t *)(uintptr_t)arg1;
        if (!args || !args->cap_id || args->opcode != BH_ENDPOINT_OP_CLOSE) return BH_ERR_INVALID_ARGUMENT;
        g_close_count++;
        return 0;
    }
    if (!arg1) return BH_ERR_INVALID_ARGUMENT;
    if (sysno == SYSCALL_ENDPOINT_CREATE) {
        g_create_count++;
        const bharat_sys_endpoint_create_args_t *args = (void *)(uintptr_t)arg1;
        if (!args->out_send_cap_ptr || !args->out_recv_cap_ptr) return BH_ERR_INVALID_ARGUMENT;
        *(uint32_t *)(uintptr_t)args->out_send_cap_ptr = 2;
        *(uint32_t *)(uintptr_t)args->out_recv_cap_ptr = 3;
        return 0;
    }
    if (sysno == SYSCALL_ENDPOINT_SEND) {
        const bharat_sys_endpoint_send_args_t *args = (void *)(uintptr_t)arg1;
        if (!args->send_cap || !args->payload_ptr || args->payload_len > 128) return BH_ERR_INVALID_ARGUMENT;
        g_last_sent_cap = args->cap_to_send;
        g_last_sent_reply_cap = args->reply_cap_to_send;
        g_last_sent_reply_rights = args->reply_cap_send_rights;
        return bharat_ipc_transport_send(args->send_cap, (void *)(uintptr_t)args->payload_ptr,
                                        args->payload_len, args->timeout_ticks);
    }
    if (sysno == SYSCALL_ENDPOINT_RECEIVE) {
        if (arg2 == (long)BH_IPC_RECEIVE_SELECTOR_V1) {
            const bharat_sys_endpoint_receive_v1_args_t *args = (void *)(uintptr_t)arg1;
            if (args->version != BH_IPC_RECEIVE_ENVELOPE_VERSION_V1 ||
                args->struct_size != sizeof(*args) ||
                !args->recv_cap || !args->out_payload_ptr || !args->out_len_ptr || !args->out_envelope_ptr ||
                args->out_envelope_capacity < sizeof(bh_ipc_receive_envelope_v1_t)) {
                return BH_ERR_INVALID_ARGUMENT;
            }
            *(uint32_t *)(uintptr_t)args->out_len_ptr = 0;
            bh_ipc_receive_envelope_v1_t *env = (bh_ipc_receive_envelope_v1_t *)(uintptr_t)args->out_envelope_ptr;
            memset(env, 0, sizeof(*env));
            env->version = BH_IPC_RECEIVE_ENVELOPE_VERSION_V1;
            env->struct_size = sizeof(*env);
            long st = bharat_ipc_transport_receive(args->recv_cap, (void *)(uintptr_t)args->out_payload_ptr,
                args->out_payload_capacity, (uint32_t *)(uintptr_t)args->out_len_ptr, args->timeout_ticks);
            if (st == 0 && g_pending_cap != 0U) {
                env->authority_cap = g_pending_cap;
                env->valid_fields |= BH_IPC_ENVELOPE_F_AUTHORITY_CAP;
                g_pending_cap = 0U;
            }
            return st;
        }
        const bharat_sys_endpoint_receive_args_t *args = (void *)(uintptr_t)arg1;
        if (!args->recv_cap || !args->out_payload_ptr || !args->out_len_ptr) return BH_ERR_INVALID_ARGUMENT;
        *(uint32_t *)(uintptr_t)args->out_len_ptr = 0;
        /* The legacy data-only wrapper must decline transfers so the kernel
         * owns their disposal. This mock checks the call shape, not provenance. */
        assert(args->out_received_cap_ptr == 0U);
        if (g_pending_cap != 0U) {
            g_discarded_caps++;
            g_pending_cap = 0U;
        }
        return bharat_ipc_transport_receive(args->recv_cap, (void *)(uintptr_t)args->out_payload_ptr,
            args->out_payload_capacity, (uint32_t *)(uintptr_t)args->out_len_ptr, args->timeout_ticks);
    }
    return BH_ERR_INVALID_ARGUMENT;
}

static bharat_ipc_msg_header_t mk_hdr(uint64_t msg_id, uint32_t payload_size) {
    bharat_ipc_msg_header_t hdr = {0};
    hdr.header_version = BHARAT_IPC_HEADER_VERSION_V1;
    hdr.service_id = 1U;
    hdr.interface_id = 1U;
    hdr.interface_version = 1U;
    hdr.opcode = 42U;
    hdr.flags = 0U;
    hdr.payload_size = payload_size;
    hdr.status = BHARAT_IPC_STATUS_OK;
    hdr.message_id = msg_id;
    return hdr;
}

static void clear_state() {
    g_rx_head = 0;
    g_rx_tail = 0;
    g_next_send_status = 0;
    g_next_recv_status = 0;
    g_create_count = 0;
    g_close_count = 0;
    g_send_count = 0;
    g_recv_count = 0;
    g_discarded_caps = 0;
    g_pending_cap = 0;
    g_last_sent_cap = 0;
    g_last_sent_reply_cap = 0;
    g_last_sent_reply_rights = 0;
    g_last_send_timeout = 0;
    g_last_recv_timeout = 0;
    memset(&g_last_sent, 0, sizeof(g_last_sent));
}

int main(void) {
    printf("Starting IPC wire format tests...\n");
    clear_state();

    bharat_ipc_msg_header_t hdr = mk_hdr(1, 0);
    int32_t st;
    char buf[128] = {0};
    uint8_t wire_buf[128] = {0};

    // 1. null headers
    st = bharat_ipc_send_ex(1, NULL, NULL, 0);
    assert(st == BHARAT_IPC_STATUS_ERR_DECODE);

    st = bharat_ipc_recv_ex(1, NULL, NULL, 0, 0);
    assert(st == BHARAT_IPC_STATUS_ERR_DECODE);

    // 2. null payloads
    hdr.payload_size = 4;
    st = bharat_ipc_send_ex(1, &hdr, NULL, 0);
    assert(st == BHARAT_IPC_STATUS_ERR_DECODE);

    st = bharat_ipc_recv_ex(1, &hdr, NULL, 4, 0);
    assert(st == BHARAT_IPC_STATUS_ERR_DECODE);

    // 3. invalid header versions
    hdr = mk_hdr(1, 0);
    hdr.header_version = 2; // Invalid
    st = bharat_ipc_send_ex(1, &hdr, NULL, 0);
    assert(st == BHARAT_IPC_STATUS_ERR_VERSION);

    // 4. zero-length payloads
    hdr = mk_hdr(1, 0);
    st = bharat_ipc_send_ex(1, &hdr, NULL, 0);
    assert(st == BHARAT_IPC_STATUS_OK);
    assert(g_last_sent.len == sizeof(bharat_ipc_msg_header_t));

    // 5. maximum supported payload
    uint32_t max_payload = 128 - sizeof(bharat_ipc_msg_header_t);
    hdr = mk_hdr(1, max_payload);
    st = bharat_ipc_send_ex(1, &hdr, buf, 0);
    assert(st == BHARAT_IPC_STATUS_OK);
    assert(g_last_sent.len == 128);

    // 6. oversized payload
    hdr = mk_hdr(1, max_payload + 1);
    st = bharat_ipc_send_ex(1, &hdr, buf, 0);
    assert(st == BHARAT_IPC_STATUS_ERR_LENGTH);

    // 7. truncated receive
    hdr = mk_hdr(1, 4);
    memcpy(wire_buf, &hdr, sizeof(hdr));

    // Queue shorter than payload
    clear_state();
    queue_rx(wire_buf, sizeof(bharat_ipc_msg_header_t) + 2);
    st = bharat_ipc_recv_ex(1, &hdr, buf, 4, 0);
    assert(st == BHARAT_IPC_STATUS_ERR_LENGTH);

    // Queue shorter than header
    clear_state();
    queue_rx(wire_buf, sizeof(bharat_ipc_msg_header_t) - 1);
    st = bharat_ipc_recv_ex(1, &hdr, buf, 4, 0);
    assert(st == BHARAT_IPC_STATUS_ERR_DECODE);

    // Receive successfully (exact size)
    clear_state();
    hdr = mk_hdr(1, 4);
    memcpy(wire_buf, &hdr, sizeof(hdr));
    queue_rx(wire_buf, sizeof(bharat_ipc_msg_header_t) + 4);
    st = bharat_ipc_recv_ex(1, &hdr, buf, 4, 0);
    assert(st == BHARAT_IPC_STATUS_OK);

    // 8. inconsistent wire length (too large wire msg)
    clear_state();
    hdr = mk_hdr(1, 4);
    memcpy(wire_buf, &hdr, sizeof(hdr));
    queue_rx(wire_buf, sizeof(bharat_ipc_msg_header_t) + 8);
    st = bharat_ipc_recv_ex(1, &hdr, buf, 8, 0);
    assert(st == BHARAT_IPC_STATUS_ERR_LENGTH);

    // 9. transport errors and timeouts
    g_next_send_status = BH_ERR_ACCESS_DENIED;
    hdr = mk_hdr(1, 0);
    st = bharat_ipc_send_ex(1, &hdr, NULL, 0);
    assert(st == BHARAT_IPC_STATUS_ERR_PERM);

    g_next_recv_status = BH_ERR_TIMEOUT;
    st = bharat_ipc_recv_ex(1, &hdr, buf, 4, 0);
    assert(st == BHARAT_IPC_STATUS_ERR_TIMEOUT);

    // The native syscall status namespace differs from kernel IPC enums.
    const struct {
        long native_status;
        int32_t service_status;
    } status_cases[] = {
        {BH_ERR_INVALID_ARGUMENT, BHARAT_IPC_STATUS_ERR_INVALID},
        {BH_ERR_INVALID_SYSCALL, BHARAT_IPC_STATUS_ERR_UNSUPPORTED},
        {BH_ERR_NOT_SUPPORTED, BHARAT_IPC_STATUS_ERR_UNSUPPORTED},
        {BH_ERR_ACCESS_DENIED, BHARAT_IPC_STATUS_ERR_PERM},
        {BH_ERR_FAULT, BHARAT_IPC_STATUS_ERR_INTERNAL},
        {BH_ERR_NO_MEMORY, BHARAT_STATUS_ERR_NO_MEMORY},
        {BH_ERR_TRY_AGAIN, BHARAT_IPC_STATUS_ERR_BUSY},
        {BH_ERR_INTERRUPTED, BHARAT_IPC_STATUS_ERR_INTERNAL},
        {BH_ERR_BAD_STATE, BHARAT_STATUS_ERR_BAD_STATE},
        {BH_ERR_TIMEOUT, BHARAT_IPC_STATUS_ERR_TIMEOUT},
        {BH_ERR_OVERFLOW, BHARAT_IPC_STATUS_ERR_LENGTH},
        {BH_ERR_INTERNAL, BHARAT_IPC_STATUS_ERR_INTERNAL},
        {BH_ERR_BAD_CAPABILITY, BHARAT_IPC_STATUS_ERR_PERM},
        {BH_ERR_STALE_CAPABILITY, BHARAT_IPC_STATUS_ERR_PERM},
        {BH_ERR_WRONG_TYPE, BHARAT_IPC_STATUS_ERR_PERM},
        {BH_ERR_INSUFFICIENT_RIGHTS, BHARAT_IPC_STATUS_ERR_PERM},
        {BH_ERR_CHANNEL_CLOSED, BHARAT_IPC_STATUS_ERR_ENDPOINT},
        {BH_ERR_BUFFER_FULL, BHARAT_IPC_STATUS_ERR_BUSY},
        {BH_ERR_RESOURCE_EXHAUSTED, BHARAT_STATUS_ERR_NO_MEMORY},
        {BH_ERR_NOT_FOUND, BHARAT_IPC_STATUS_ERR_ENDPOINT},
        {-999, BHARAT_IPC_STATUS_ERR_INTERNAL},
    };
    for (uint32_t i = 0; i < sizeof(status_cases) / sizeof(status_cases[0]); ++i) {
        clear_state();
        hdr = mk_hdr(1, 0);
        bharat_ipc_msg_header_t before = hdr;
        g_next_send_status = status_cases[i].native_status;
        st = bharat_ipc_send_ex(1, &hdr, NULL, 0);
        assert(st == status_cases[i].service_status);
        g_next_recv_status = status_cases[i].native_status;
        st = bharat_ipc_recv_ex(1, &hdr, NULL, 0, 0);
        assert(st == status_cases[i].service_status);
        assert(memcmp(&hdr, &before, sizeof(hdr)) == 0);
    }

    // Unknown flags fail before outputs or transport are published.
    clear_state();
    hdr = mk_hdr(2, 0);
    hdr.flags = UINT32_C(1) << 31;
    st = bharat_ipc_send_ex(1, &hdr, NULL, 123);
    assert(st == BHARAT_IPC_STATUS_ERR_FLAGS);
    assert(g_send_count == 0);
    memcpy(wire_buf, &hdr, sizeof(hdr));
    queue_rx(wire_buf, sizeof(hdr));
    bharat_ipc_msg_header_t output = mk_hdr(777, 0);
    bharat_ipc_msg_header_t sentinel = output;
    st = bharat_ipc_recv_ex(1, &output, NULL, 0, 456);
    assert(st == BHARAT_IPC_STATUS_ERR_FLAGS);
    assert(memcmp(&output, &sentinel, sizeof(output)) == 0);
    assert(g_last_recv_timeout == 456);

    // Serialized handles never become receiver-local authority.
    clear_state();
    hdr = mk_hdr(3, 0);
    hdr.capability_transfer = UINT64_MAX;
    hdr.reply_endpoint = UINT64_MAX;
    st = bharat_ipc_send_ex(1, &hdr, NULL, 0);
    assert(st == BHARAT_IPC_STATUS_ERR_UNSUPPORTED);
    assert(g_send_count == 0);
    memcpy(wire_buf, &hdr, sizeof(hdr));
    queue_rx(wire_buf, sizeof(hdr));
    st = bharat_ipc_recv_ex(1, &output, NULL, 0, 0);
    assert(st == BHARAT_IPC_STATUS_OK);
    assert(output.capability_transfer == 0U);
    assert(output.reply_endpoint == 0U);

    // Even a real transport transfer cannot supply a reply role under V1.
    // The kernel must discard it; malformed messages cannot leak library caps.
    clear_state();
    hdr = mk_hdr(4, 0);
    hdr.flags = BHARAT_IPC_FLAG_CAP_TRANSFER;
    memcpy(wire_buf, &hdr, sizeof(hdr));
    queue_rx(wire_buf, sizeof(hdr));
    g_pending_cap = 42;
    output = sentinel;
    st = bharat_ipc_recv_ex(1, &output, NULL, 0, 0);
    assert(st == BHARAT_IPC_STATUS_ERR_UNSUPPORTED);
    assert(g_discarded_caps == 1);
    assert(memcmp(&output, &sentinel, sizeof(output)) == 0);

    clear_state();
    queue_rx(wire_buf, sizeof(hdr) - 1);
    g_pending_cap = 43;
    st = bharat_ipc_recv_ex(1, &output, NULL, 0, 0);
    assert(st == BHARAT_IPC_STATUS_ERR_DECODE);
    assert(g_discarded_caps == 1);
    assert(memcmp(&output, &sentinel, sizeof(output)) == 0);

    // Send/receive durations retain scheduler tick units without conversion.
    clear_state();
    hdr = mk_hdr(5, 0);
    st = bharat_ipc_send_ex(1, &hdr, NULL, 123);
    assert(st == BHARAT_IPC_STATUS_OK);
    assert(g_last_send_timeout == 123);

    // Invalid RPC arguments fail before allocating any endpoint.
    clear_state();
    st = bharat_ipc_call_ex(1, NULL, NULL, &output, buf, sizeof(buf), 0);
    assert(st == BHARAT_IPC_STATUS_ERR_DECODE);
    hdr = mk_hdr(10, 0);
    hdr.flags = UINT32_C(1) << 31;
    st = bharat_ipc_call_ex(1, &hdr, NULL, &output, buf, sizeof(buf), 0);
    assert(st == BHARAT_IPC_STATUS_ERR_FLAGS);
    hdr = mk_hdr(10, 1);
    st = bharat_ipc_call_ex(1, &hdr, NULL, &output, buf, sizeof(buf), 0);
    assert(st == BHARAT_IPC_STATUS_ERR_DECODE);
    hdr = mk_hdr(10, 0);
    hdr.reply_endpoint = 99;
    st = bharat_ipc_call_ex(1, &hdr, NULL, &output, buf, sizeof(buf), 0);
    assert(st == BHARAT_IPC_STATUS_ERR_UNSUPPORTED);
    assert(g_create_count == 0);

    // 10,000 RPC timeout/error iterations must deterministically close both
    // reply endpoint capabilities on every path and leave outputs untouched.
    clear_state();
    hdr = mk_hdr(10, 0);
    output = sentinel;
    memset(buf, 'x', sizeof(buf));
    for (uint32_t i = 0; i < 10000; ++i) {
        const uint64_t timeouts[] = {0U, 1U, UINT64_MAX};
        st = bharat_ipc_call_ex(1, &hdr, NULL, &output, buf,
                                sizeof(buf), timeouts[i % 3]);
        assert(st == BHARAT_IPC_STATUS_ERR_TIMEOUT);
        assert(memcmp(&output, &sentinel, sizeof(output)) == 0);
        for (uint32_t j = 0; j < sizeof(buf); ++j) assert(buf[j] == 'x');
    }
    assert(g_create_count == 10000);
    assert(g_send_count == 10000);
    assert(g_recv_count == 10000);
    assert(g_close_count == 20000);

    // Authenticated RPC round-trip with dual capability transfer (authority + reply).
    clear_state();
    hdr = mk_hdr(55, 4);
    hdr.capability_transfer = 77;
    const uint32_t req_val = 0x11223344U;
    const uint32_t rep_val = 0x55667788U;
    bharat_ipc_msg_header_t reply_hdr = mk_hdr(55, 4);
    reply_hdr.capability_transfer = 9999; /* Forged wire cap must be ignored */
    memcpy(wire_buf, &reply_hdr, sizeof(reply_hdr));
    memcpy(wire_buf + sizeof(reply_hdr), &rep_val, sizeof(rep_val));
    queue_rx(wire_buf, sizeof(reply_hdr) + sizeof(rep_val));
    g_pending_cap = 88; /* Kernel-verified envelope authority cap */
    uint32_t out_val = 0;
    st = bharat_ipc_call_ex(1, &hdr, &req_val, &output, &out_val, sizeof(out_val), 50);
    assert(st == BHARAT_IPC_STATUS_OK);
    assert(g_last_sent_cap == 77);
    assert(g_last_sent_reply_cap == 2);
    assert(g_last_sent_reply_rights == (1ULL << 2));
    assert(output.message_id == 55);
    assert(output.capability_transfer == 88);
    assert(out_val == 0x55667788U);
    assert(g_create_count == 1 && g_close_count == 2);

    printf("All IPC wire format tests passed!\n");
    return 0;
}
