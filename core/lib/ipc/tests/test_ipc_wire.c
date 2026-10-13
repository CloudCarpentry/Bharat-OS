#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "bharat/ipc/ipc.h"

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

static queued_msg_t g_last_sent;

long bharat_ipc_transport_send(uint32_t send_cap, const void *payload, uint32_t len, uint64_t timeout_ticks) {
    (void)send_cap; (void)timeout_ticks;
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
    (void)recv_cap; (void)timeout_ticks;
    if (g_next_recv_status != 0) {
        long st = g_next_recv_status;
        g_next_recv_status = 0;
        return st;
    }
    if (g_rx_head == g_rx_tail) {
        return -8; // Timeout
    }
    uint32_t idx = g_rx_head % 32;
    if (g_rx_queue[idx].len > out_capacity) {
        return -2;
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
    (void)sysno; (void)arg1; (void)arg2; (void)arg3; (void)arg4; (void)arg5; (void)arg6;
    return 0;
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
    g_next_send_status = -4; // BHARAT_IPC_STATUS_ERR_PERM
    hdr = mk_hdr(1, 0);
    st = bharat_ipc_send_ex(1, &hdr, NULL, 0);
    assert(st == BHARAT_IPC_STATUS_ERR_PERM);

    g_next_recv_status = -8; // Timeout
    st = bharat_ipc_recv_ex(1, &hdr, buf, 4, 0);
    assert(st == BHARAT_IPC_STATUS_ERR_TIMEOUT);

    // 10. call_ex
    clear_state();
    hdr = mk_hdr(10, 0); // req
    bharat_ipc_msg_header_t rep_hdr = mk_hdr(10, 0); // exact same message_id
    rep_hdr.flags = BHARAT_IPC_FLAG_REPLY;

    memcpy(wire_buf, &rep_hdr, sizeof(rep_hdr));
    queue_rx(wire_buf, sizeof(bharat_ipc_msg_header_t));

    st = bharat_ipc_call_ex(1, &hdr, NULL, &rep_hdr, buf, 0, 0);
    assert(st == BHARAT_IPC_STATUS_OK);

    // call_ex mismatch message id
    clear_state();
    rep_hdr.message_id = 99; // mismatch
    memcpy(wire_buf, &rep_hdr, sizeof(rep_hdr));
    queue_rx(wire_buf, sizeof(bharat_ipc_msg_header_t));

    st = bharat_ipc_call_ex(1, &hdr, NULL, &rep_hdr, buf, 0, 0);
    assert(st == BHARAT_IPC_STATUS_ERR_DECODE);

    // call_ex transport errors
    clear_state();
    g_next_send_status = -4; // permission error on send
    st = bharat_ipc_call_ex(1, &hdr, NULL, &rep_hdr, buf, 0, 0);
    assert(st == BHARAT_IPC_STATUS_ERR_PERM);

    clear_state();
    g_next_recv_status = -8; // timeout on recv
    st = bharat_ipc_call_ex(1, &hdr, NULL, &rep_hdr, buf, 0, 0);
    assert(st == BHARAT_IPC_STATUS_ERR_TIMEOUT);

    printf("All IPC wire format tests passed!\n");
    return 0;
}
