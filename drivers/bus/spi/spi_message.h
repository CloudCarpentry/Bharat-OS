#ifndef BHARATOS_SPI_MESSAGE_H
#define BHARATOS_SPI_MESSAGE_H

#include <stdint.h>
#include <stddef.h>

typedef struct spi_transfer {
    const void *tx_buf;
    void *rx_buf;
    size_t len;
    uint32_t speed_hz;
    uint16_t bits_per_word;
    uint8_t cs_change;
    uint8_t tx_nbits;
    uint8_t rx_nbits;
} spi_transfer_t;

typedef struct spi_message {
    spi_transfer_t *transfers;
    size_t num_transfers;
    void *context;
    void (*complete)(void *context);
    int status;
} spi_message_t;

#endif /* BHARATOS_SPI_MESSAGE_H */