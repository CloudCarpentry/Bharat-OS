#ifndef BHARATOS_SPI_DEVICE_H
#define BHARATOS_SPI_DEVICE_H

#include <stdint.h>

struct spi_controller;

#define SPI_CPHA 0x01
#define SPI_CPOL 0x02
#define SPI_CS_HIGH 0x04
#define SPI_LSB_FIRST 0x08
#define SPI_3WIRE 0x10
#define SPI_LOOP 0x20

typedef struct spi_device {
    struct spi_controller *controller;
    uint32_t max_speed_hz;
    uint8_t chip_select;
    uint8_t bits_per_word;
    uint16_t mode;
    char name[32];
    void *driver_data;
} spi_device_t;

#endif /* BHARATOS_SPI_DEVICE_H */