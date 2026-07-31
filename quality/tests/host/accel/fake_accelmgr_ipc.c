#include "fake_accelmgr_ipc.h"
#include <string.h>

static bh_test_ipc_state_t g_ipc;

void bh_test_ipc_reset(void) {
    memset(&g_ipc, 0, sizeof(g_ipc));
}

bh_test_ipc_state_t *bh_test_ipc_state(void) {
    return &g_ipc;
}

int32_t bharat_ipc_send(
    bharat_ipc_endpoint_t endpoint,
    const bharat_ipc_msg_header_t *header,
    const void *payload)
{
    g_ipc.call_count++;
    g_ipc.last_endpoint = endpoint;

    if (header != NULL) {
        g_ipc.last_header = *header;
        g_ipc.last_payload_size = header->payload_size;

        if (payload != NULL && header->payload_size <= sizeof(g_ipc.last_payload)) {
            memcpy(g_ipc.last_payload, payload, header->payload_size);
        }
    }

    return g_ipc.configured_result;
}
