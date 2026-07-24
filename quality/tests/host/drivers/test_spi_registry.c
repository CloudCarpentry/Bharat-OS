#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "../../../core/drivers/bus/spi/spi_registry.h"

int mock_transfer_one_message(struct spi_controller *ctrl, spi_message_t *msg) {
    (void)ctrl;
    (void)msg;
    return 0; // Success
}

int main(void) {
    printf("Running SPI registry tests...\n");

    extern void spi_registry_init(void);
    spi_registry_init();

    spi_controller_ops_t ops = {
        .setup = NULL,
        .transfer_one_message = mock_transfer_one_message,
        .cleanup = NULL
    };

    spi_controller_t ctrl = {
        .bus_num = 0,
        .num_chipselect = 2,
        .ops = &ops,
        .name = "mock_spi"
    };

    // Test successful registration
    int ret = spi_controller_register(&ctrl);
    assert(ret == 0);
    printf("Registered valid SPI controller successfully.\n");

    // Test duplicate registration
    ret = spi_controller_register(&ctrl);
    assert(ret != 0); // Should fail
    printf("Duplicate SPI registration handled correctly.\n");

    // Test get controller
    spi_controller_t *fetched = spi_get_controller(0);
    assert(fetched == &ctrl);
    printf("Fetched SPI controller correctly.\n");

    // Test invalid bus ID
    fetched = spi_get_controller(-1);
    assert(fetched == NULL);
    fetched = spi_get_controller(100);
    assert(fetched == NULL);
    printf("Handled invalid SPI bus IDs correctly.\n");

    // Test unregister
    spi_controller_unregister(&ctrl);
    fetched = spi_get_controller(0);
    assert(fetched == NULL);
    printf("Unregistered SPI controller successfully.\n");

    // Test invalid controller registration
    spi_controller_t invalid_ctrl_1 = {
        .bus_num = 1,
        .ops = NULL // Missing ops
    };
    ret = spi_controller_register(&invalid_ctrl_1);
    assert(ret != 0);

    spi_controller_ops_t invalid_ops = {
        .transfer_one_message = NULL // Missing transfer_one_message
    };
    spi_controller_t invalid_ctrl_2 = {
        .bus_num = 2,
        .ops = &invalid_ops
    };
    ret = spi_controller_register(&invalid_ctrl_2);
    assert(ret != 0);
    printf("Invalid SPI controller registrations rejected correctly.\n");

    printf("All SPI registry tests passed!\n");
    return 0;
}