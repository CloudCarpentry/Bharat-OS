#include <stdio.h>
#include <assert.h>
#include "../../../core/drivers/bus/spi/spi_registry.h"
#include "../../../core/drivers/bus/spi/spi_core.h"
#include <string.h>

static int mock_transfer_called = 0;
static spi_message_t *last_msg = NULL;

static int mock_transfer_one_message(spi_controller_t *ctrl, spi_message_t *msg) {
    (void)ctrl;
    mock_transfer_called = 1;
    last_msg = msg;
    return 0; // Success
}

int main(void) {
    printf("Running SPI transfer tests...\n");
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

    spi_controller_register(&ctrl);

    // Mock client setup
    spi_device_t spi = {
        .controller = &ctrl,
        .chip_select = 0,
        .max_speed_hz = 1000000,
        .bits_per_word = 8,
        .mode = 0,
        .name = "mock_spi_device"
    };

    // Test successful simple transfer
    char tx_buf[] = "test";
    char rx_buf[4];
    int ret = spi_transfer_simple(&spi, tx_buf, rx_buf, sizeof(tx_buf));
    assert(ret == 0);
    assert(mock_transfer_called == 1);
    assert(last_msg != NULL);
    assert(last_msg->num_transfers == 1);
    assert(last_msg->transfers[0].tx_buf == tx_buf);
    assert(last_msg->transfers[0].rx_buf == rx_buf);
    assert(last_msg->transfers[0].len == sizeof(tx_buf));

    printf("SPI simple transfer successfully triggered controller xfer.\n");

    // Reset mock
    mock_transfer_called = 0;
    last_msg = NULL;

    // Test successful direct sync
    spi_transfer_t t = {
        .tx_buf = tx_buf,
        .rx_buf = NULL,
        .len = sizeof(tx_buf),
    };
    spi_message_t m = {
        .transfers = &t,
        .num_transfers = 1,
    };

    ret = spi_sync(&spi, &m);
    assert(ret == 0);
    assert(mock_transfer_called == 1);
    assert(last_msg == &m);

    printf("SPI direct sync successfully triggered controller xfer.\n");

    // Reset mock
    mock_transfer_called = 0;
    last_msg = NULL;

    // Test invalid chip select
    spi_device_t invalid_spi = spi;
    invalid_spi.chip_select = 5; // > num_chipselect (2)
    ret = spi_transfer_simple(&invalid_spi, tx_buf, rx_buf, sizeof(tx_buf));
    assert(ret != 0);
    printf("Handled invalid chip select correctly.\n");

    // Test null device
    ret = spi_transfer_simple(NULL, tx_buf, rx_buf, sizeof(tx_buf));
    assert(ret != 0);

    // Test invalid num
    ret = spi_transfer_simple(&spi, tx_buf, rx_buf, 0);
    assert(ret != 0);

    // Test invalid buffer (both null)
    ret = spi_transfer_simple(&spi, NULL, NULL, sizeof(tx_buf));
    assert(ret != 0);
    printf("Handled invalid transfer parameters correctly.\n");

    spi_controller_unregister(&ctrl);

    printf("All SPI transfer tests passed!\n");
    return 0;
}