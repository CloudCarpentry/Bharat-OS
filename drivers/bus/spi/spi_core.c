#include "spi_core.h"
#include <stddef.h>

int spi_setup(spi_device_t *spi) {
    if (!spi || !spi->controller || !spi->controller->ops) return -1;
    if (spi->chip_select >= spi->controller->num_chipselect) return -1;

    if (spi->controller->ops->setup) {
        return spi->controller->ops->setup(spi);
    }
    return 0; // Success if no specific setup ops required
}

int spi_sync(spi_device_t *spi, spi_message_t *message) {
    if (!spi || !spi->controller || !spi->controller->ops || !message) return -1;
    if (spi->chip_select >= spi->controller->num_chipselect) return -1;

    if (!message->transfers || message->num_transfers == 0) return -1;

    for (size_t i = 0; i < message->num_transfers; ++i) {
        if (!message->transfers[i].tx_buf && !message->transfers[i].rx_buf) {
            return -1; // Either tx or rx must be present
        }
    }

    if (spi->controller->ops->transfer_one_message) {
        int ret = spi->controller->ops->transfer_one_message(spi->controller, message);
        if (message->complete) {
            message->complete(message->context);
        }
        return ret;
    }

    return -1; // Unsupported
}

int spi_transfer_simple(spi_device_t *spi, const void *tx_buf, void *rx_buf, size_t len) {
    if (!spi || len == 0) return -1;
    if (!tx_buf && !rx_buf) return -1;

    spi_transfer_t t = {
        .tx_buf = tx_buf,
        .rx_buf = rx_buf,
        .len = len,
        .speed_hz = spi->max_speed_hz,
        .bits_per_word = spi->bits_per_word,
        .cs_change = 1,
        .tx_nbits = 1,
        .rx_nbits = 1
    };

    spi_message_t m = {
        .transfers = &t,
        .num_transfers = 1,
        .context = NULL,
        .complete = NULL,
        .status = 0
    };

    return spi_sync(spi, &m);
}