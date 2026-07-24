#include <stdio.h>
#include <assert.h>
#include "../../../core/drivers/bus/i2c/i2c_registry.h"
#include "../../../core/drivers/bus/i2c/i2c_core.h"
#include <string.h>

static int mock_transfer_called = 0;
static i2c_msg_t *last_msg = NULL;
static int last_num = 0;

static int mock_master_xfer(i2c_controller_t *ctrl, i2c_msg_t *msgs, int num) {
    (void)ctrl;
    mock_transfer_called = 1;
    last_msg = msgs;
    last_num = num;
    return 0; // Success
}

int main(void) {
    printf("Running I2C transfer tests...\n");
    extern void i2c_registry_init(void);
    i2c_registry_init();

    i2c_controller_ops_t ops = {
        .master_xfer = mock_master_xfer,
        .functionality = NULL
    };

    i2c_controller_t ctrl = {
        .bus_id = 0,
        .name = "mock_i2c",
        .ops = &ops,
        .dev_data = NULL
    };

    i2c_controller_register(&ctrl);

    // Mock client setup
    i2c_device_t client = {
        .controller = &ctrl,
        .addr = 0x50,
        .name = "mock_client",
        .driver_data = NULL
    };

    // Test successful master send
    char send_buf[] = "test";
    int ret = i2c_master_send(&client, send_buf, sizeof(send_buf));
    assert(ret == 0);
    assert(mock_transfer_called == 1);
    assert(last_num == 1);
    assert(last_msg != NULL);
    assert(last_msg->addr == 0x50);
    assert(last_msg->len == sizeof(send_buf));
    assert(last_msg->buf == (uint8_t *)send_buf);
    assert(last_msg->flags == 0); // Send flag should be 0

    printf("Master send successfully triggered controller xfer.\n");

    // Reset mock
    mock_transfer_called = 0;
    last_msg = NULL;
    last_num = 0;

    // Test successful master receive
    char recv_buf[4];
    ret = i2c_master_recv(&client, recv_buf, sizeof(recv_buf));
    assert(ret == 0);
    assert(mock_transfer_called == 1);
    assert(last_num == 1);
    assert(last_msg != NULL);
    assert(last_msg->addr == 0x50);
    assert(last_msg->len == sizeof(recv_buf));
    assert(last_msg->buf == (uint8_t *)recv_buf);
    assert(last_msg->flags == I2C_M_RD); // Receive flag should be set

    printf("Master recv successfully triggered controller xfer.\n");

    // Reset mock
    mock_transfer_called = 0;
    last_msg = NULL;
    last_num = 0;

    // Test direct transfer
    i2c_msg_t msgs[2];
    msgs[0].addr = 0x50; msgs[0].flags = 0; msgs[0].len = 1; msgs[0].buf = (uint8_t *)"a";
    msgs[1].addr = 0x50; msgs[1].flags = I2C_M_RD; msgs[1].len = 1; msgs[1].buf = (uint8_t *)recv_buf;

    ret = i2c_transfer(&ctrl, msgs, 2);
    assert(ret == 0);
    assert(mock_transfer_called == 1);
    assert(last_num == 2);
    assert(last_msg == msgs);
    printf("Direct transfer triggered correctly.\n");

    // Test null controller
    ret = i2c_transfer(NULL, msgs, 2);
    assert(ret != 0);

    // Test null msgs
    ret = i2c_transfer(&ctrl, NULL, 2);
    assert(ret != 0);

    // Test invalid num
    ret = i2c_transfer(&ctrl, msgs, 0);
    assert(ret != 0);
    printf("Handled invalid transfer parameters correctly.\n");

    i2c_controller_unregister(&ctrl);

    printf("All I2C transfer tests passed!\n");
    return 0;
}