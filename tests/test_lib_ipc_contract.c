#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "../lib/ipc/include/bharat/ipc/ipc.h"

typedef struct {
    uint8_t bytes[128];
    uint32_t len;
} queued_msg_t;

static queued_msg_t g_queue[32];
static uint32_t g_head;
static uint32_t g_tail;
static long g_next_send_status;
static long g_next_recv_status;

long bharat_ipc_transport_send(uint32_t send_cap, const void *payload, uint32_t len, uint64_t timeout_ticks) {
    (void)timeout_ticks;
    if (g_next_send_status != 0) {
        long st = g_next_send_status;
        g_next_send_status = 0;
        return st;
    }
    if (send_cap == 0U) {
        return -7; /* closed/invalid endpoint */
    }
    if (len > sizeof(g_queue[0].bytes) || (g_tail - g_head) >= 32U) {
        return -2;
    }
    uint32_t idx = g_tail % 32U;
    memcpy(g_queue[idx].bytes, payload, len);
    g_queue[idx].len = len;
    g_tail++;
    return 0;
}

long bharat_ipc_transport_receive(uint32_t recv_cap, void *out_payload, uint32_t out_capacity, uint32_t *out_len, uint64_t timeout_ticks) {
    (void)timeout_ticks;
    if (g_next_recv_status != 0) {
        long st = g_next_recv_status;
        g_next_recv_status = 0;
        return st;
    }
    if (recv_cap == 0U) {
        return -4; /* permission denied */
    }
    if (g_head == g_tail) {
        return -8; /* timeout / no message */
    }
    uint32_t idx = g_head % 32U;
    if (g_queue[idx].len > out_capacity) {
        return -2;
    }
    memcpy(out_payload, g_queue[idx].bytes, g_queue[idx].len);
    *out_len = g_queue[idx].len;
    g_head++;
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

int main(void) {
    const char req[] = "ping";
    const char rep[] = "pong";
    char out[16] = {0};
    bharat_ipc_msg_header_t req_hdr = mk_hdr(100U, 4U);
    bharat_ipc_msg_header_t rep_hdr = mk_hdr(100U, 4U);
    rep_hdr.flags = BHARAT_IPC_FLAG_REPLY;

    /* Basic request/response */
    assert(bharat_ipc_send_ex(1U, &req_hdr, req, 5U) == BHARAT_IPC_STATUS_OK);
    assert(bharat_ipc_recv_ex(1U, &rep_hdr, out, sizeof(out), 5U) == BHARAT_IPC_STATUS_OK);
    assert(memcmp(out, req, 4U) == 0);

    /* queue a synthetic reply and test call path + correlation */
    assert(bharat_ipc_send_ex(1U, &rep_hdr, rep, 5U) == BHARAT_IPC_STATUS_OK);
    memset(out, 0, sizeof(out));
    assert(bharat_ipc_call_ex(1U, &req_hdr, req, &rep_hdr, out, sizeof(out), 5U) == BHARAT_IPC_STATUS_OK);
    assert(memcmp(out, rep, 4U) == 0);

    /* Timeout */
    g_next_recv_status = -8;
    assert(bharat_ipc_recv_ex(1U, &rep_hdr, out, sizeof(out), 1U) == BHARAT_IPC_STATUS_ERR_TIMEOUT);

    /* Invalid/closed endpoint */
    assert(bharat_ipc_send_ex(0U, &req_hdr, req, 0U) == BHARAT_IPC_STATUS_ERR_ENDPOINT);

    /* Authorization negative path */
    assert(bharat_ipc_recv_ex(0U, &rep_hdr, out, sizeof(out), 0U) == BHARAT_IPC_STATUS_ERR_PERM);

    /* Burst smoke */
    g_head = g_tail = 0U;
    for (uint64_t i = 0; i < 10; ++i) {
        bharat_ipc_msg_header_t h = mk_hdr(i + 1000U, 4U);
        assert(bharat_ipc_send_ex(1U, &h, req, 0U) == BHARAT_IPC_STATUS_OK);
    }
    for (uint64_t i = 0; i < 10; ++i) {
        bharat_ipc_msg_header_t h = {0};
        memset(out, 0, sizeof(out));
        assert(bharat_ipc_recv_ex(1U, &h, out, sizeof(out), 0U) == BHARAT_IPC_STATUS_OK);
        assert(h.message_id == (i + 1000U));
    }

    printf("lib/ipc contract tests passed.\n");
    return 0;
}
