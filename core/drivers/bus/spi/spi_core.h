#ifndef BHARATOS_SPI_CORE_H
#define BHARATOS_SPI_CORE_H

#include "spi_message.h"
#include "spi_device.h"

typedef struct spi_controller_ops {
    int (*setup)(struct spi_device *spi);
    int (*transfer_one_message)(struct spi_controller *ctrl, spi_message_t *msg);
    void (*cleanup)(struct spi_device *spi);
} spi_controller_ops_t;

typedef struct spi_controller {
    int bus_num;
    uint16_t num_chipselect;
    uint16_t mode_bits;
    const spi_controller_ops_t *ops;
    char name[32];
    void *dev_data;
} spi_controller_t;

int spi_controller_register(spi_controller_t *ctrl);
void spi_controller_unregister(spi_controller_t *ctrl);

int spi_setup(spi_device_t *spi);
int spi_sync(spi_device_t *spi, spi_message_t *message);
int spi_transfer_simple(spi_device_t *spi, const void *tx_buf, void *rx_buf, size_t len);

#endif /* BHARATOS_SPI_CORE_H */