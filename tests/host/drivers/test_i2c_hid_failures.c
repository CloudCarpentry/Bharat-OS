#include <stdio.h>
#include <assert.h>
#include "../../../drivers/input/i2c_hid/i2c_hid.h"
#include <string.h>

static int mock_i2c_state = 0; // 0: Init, 1: Reading HID desc

static int mock_master_xfer_fail(i2c_controller_t *ctrl, i2c_msg_t *msgs, int num) {
    (void)ctrl;
    if (num == 1) {
        if (msgs[0].flags == 0 && msgs[0].len == 2) {
            // Command to read register 0x0001
            if (msgs[0].buf[0] == 0x01 && msgs[0].buf[1] == 0x00) {
                mock_i2c_state = 1;
                return 0; // Success
            }
        } else if (msgs[0].flags == I2C_M_RD && mock_i2c_state == 1) {
            // Send back a malformed HID descriptor
            if (msgs[0].len == sizeof(i2c_hid_desc_t)) {
                i2c_hid_desc_t *desc = (i2c_hid_desc_t *)msgs[0].buf;
                memset(desc, 0, sizeof(*desc));
                desc->wHIDDescLength = 0x0000; // Invalid length (should be 0x001E)
                desc->bcdVersion = 0x0100;
                desc->wVendorID = 0x1234;
                desc->wProductID = 0x5678;
                return 0; // Success
            }
        }
    }
    return -1; // Failure
}

int main(void) {
    printf("Running I2C HID mock failure tests...\n");

    i2c_controller_ops_t ops = {
        .master_xfer = mock_master_xfer_fail,
        .functionality = NULL
    };

    i2c_controller_t ctrl = {
        .bus_id = 0,
        .name = "mock_i2c",
        .ops = &ops,
        .dev_data = NULL
    };

    i2c_device_t client = {
        .controller = &ctrl,
        .addr = 0x50,
        .name = "mock_client",
        .driver_data = NULL
    };

    i2c_hid_device_t hid_dev;
    int ret = i2c_hid_probe(&client, &hid_dev);
    assert(ret != 0);
    assert(hid_dev.i2c == NULL);

    printf("I2C HID probe successfully failed with malformed descriptor.\n");

    // Null safety tests
    ret = i2c_hid_probe(NULL, &hid_dev);
    assert(ret != 0);
    ret = i2c_hid_probe(&client, NULL);
    assert(ret != 0);

    i2c_hid_remove(NULL); // Should not crash
    printf("I2C HID null safety tests passed.\n");

    printf("All I2C HID failure tests passed!\n");
    return 0;
}