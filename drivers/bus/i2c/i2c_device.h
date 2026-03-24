#ifndef BHARATOS_I2C_DEVICE_H
#define BHARATOS_I2C_DEVICE_H

#include <stdint.h>
#include "i2c_msg.h"

struct i2c_controller;

typedef struct i2c_device {
    struct i2c_controller *controller;
    uint16_t addr;
    char name[32];
    void *driver_data;
} i2c_device_t;

#endif /* BHARATOS_I2C_DEVICE_H */