#ifndef BHARATOS_I2C_REGISTRY_H
#define BHARATOS_I2C_REGISTRY_H

#include "i2c_core.h"

i2c_controller_t *i2c_get_controller(int bus_id);

void i2c_registry_init(void);

i2c_device_t *i2c_device_create(i2c_controller_t *ctrl, uint16_t addr, const char *name);
void i2c_device_destroy(i2c_device_t *device);

#endif /* BHARATOS_I2C_REGISTRY_H */