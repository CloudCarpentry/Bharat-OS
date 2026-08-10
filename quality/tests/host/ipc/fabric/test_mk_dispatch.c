#include <assert.h>
#include <stdio.h>
#include "ipc/mk_proto.h"
#include "fake_hal.h"

static int g_handler_called = 0;
static const bh_mk_wire_message_t *g_received_msg = NULL;

static kstatus_t mock_handler(
    bh_mk_endpoint_handle_t source_endpoint,
    const bh_mk_wire_message_t *message,
    void *ctx)
{
    (void)source_endpoint; (void)ctx;
    g_handler_called++;
    g_received_msg = message;
    return K_OK;
}

static int g_notified_core = -1;
static kstatus_t mock_notify(uint32_t destination_core) {
    g_notified_core = (int)destination_core;
    return K_OK;
}

int main(void) {
    printf("Running test_mk_dispatch...\n");

    kstatus_t st = bh_mk_fabric_init(2);
    assert(st == K_OK);

    // Register custom doorbell notify
    bh_mk_doorbell_ops_t db_ops = { .notify = mock_notify };
    bh_mk_register_doorbell(&db_ops);

    // Core 1 registers endpoint
    fake_hal_set_cpu_id(1);
    bh_mk_endpoint_handle_t ep1;
    bh_mk_endpoint_config_t config = {
        .handler_fn = mock_handler,
        .ctx = NULL,
        .message_class = 12,
        .opcode = 34
    };
    st = bh_mk_endpoint_bind(&config, &ep1);
    assert(st == K_OK);

    // Core 0 sends message to Core 1 ep1
    fake_hal_set_cpu_id(0);
    uint32_t payload_data = 0xDEADBEEF;
    st = bh_mk_send(BH_MK_ENDPOINT_LEGACY, ep1, 12, 34, BH_MK_LANE_NORMAL, &payload_data, sizeof(payload_data), NULL);
    assert(st == K_OK);

    // Check that Core 1 was notified via doorbell
    assert(g_notified_core == 1);

    // Core 1 drains local queue
    fake_hal_set_cpu_id(1);
    assert(g_handler_called == 0);
    st = bh_mk_drain_local(10);
    assert(st == K_OK);

    // Verify handler was called and message payload received correctly
    assert(g_handler_called == 1);
    assert(g_received_msg != NULL);
    assert(g_received_msg->header.message_class == 12);
    assert(g_received_msg->header.opcode == 34);
    assert(*(uint32_t*)g_received_msg->payload == 0xDEADBEEF);

    printf("test_mk_dispatch PASSED\n");
    return 0;
}
