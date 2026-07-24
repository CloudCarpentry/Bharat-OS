#ifndef BHARATOS_SPI_REGISTRY_H
#define BHARATOS_SPI_REGISTRY_H

#include "spi_core.h"

void spi_registry_init(void);

spi_controller_t *spi_get_controller(int bus_num);
spi_device_t *spi_device_create(spi_controller_t *ctrl, uint8_t chip_select, const char *name);
void spi_device_destroy(spi_device_t *spi);

#endif /* BHARATOS_SPI_REGISTRY_H */