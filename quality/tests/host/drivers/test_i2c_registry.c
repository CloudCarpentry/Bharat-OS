#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "../../../drivers/bus/i2c/i2c_registry.h"

int mock_master_xfer(i2c_controller_t *ctrl, i2c_msg_t *msgs, int num) {
    (void)ctrl;
    (void)msgs;
    (void)num;
    return 0; // Success
}

int main(void) {
    printf("Running I2C registry tests...\n");

    // Needs manual init since this is host testing
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

    // Test successful registration
    int ret = i2c_controller_register(&ctrl);
    assert(ret == 0);
    printf("Registered valid controller successfully.\n");

    // Test duplicate registration
    ret = i2c_controller_register(&ctrl);
    assert(ret != 0); // Should fail
    printf("Duplicate registration handled correctly.\n");

    // Test get controller
    i2c_controller_t *fetched = i2c_get_controller(0);
    assert(fetched == &ctrl);
    printf("Fetched controller correctly.\n");

    // Test invalid bus ID
    fetched = i2c_get_controller(-1);
    assert(fetched == NULL);
    fetched = i2c_get_controller(100);
    assert(fetched == NULL);
    printf("Handled invalid bus IDs correctly.\n");

    // Test unregister
    i2c_controller_unregister(&ctrl);
    fetched = i2c_get_controller(0);
    assert(fetched == NULL);
    printf("Unregistered controller successfully.\n");

    // Test invalid controller registration
    i2c_controller_t invalid_ctrl_1 = {
        .bus_id = 1,
        .ops = NULL // Missing ops
    };
    ret = i2c_controller_register(&invalid_ctrl_1);
    assert(ret != 0);

    i2c_controller_ops_t invalid_ops = {
        .master_xfer = NULL // Missing master_xfer
    };
    i2c_controller_t invalid_ctrl_2 = {
        .bus_id = 2,
        .ops = &invalid_ops
    };
    ret = i2c_controller_register(&invalid_ctrl_2);
    assert(ret != 0);
    printf("Invalid controller registrations rejected correctly.\n");

    printf("All I2C registry tests passed!\n");
    return 0;
}