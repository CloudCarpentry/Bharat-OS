#ifndef BHARATOS_I2C_HID_H
#define BHARATOS_I2C_HID_H

#include "../../bus/i2c/i2c_core.h"
#include "../../bus/i2c/i2c_registry.h"
#include "i2c_hid_desc.h"

typedef struct {
    i2c_device_t *i2c;
    i2c_hid_desc_t hid_desc;
    uint8_t state;
} i2c_hid_device_t;

int i2c_hid_probe(i2c_device_t *client, i2c_hid_device_t *out_dev);
void i2c_hid_remove(i2c_hid_device_t *dev);
int i2c_hid_parse_descriptor(i2c_hid_device_t *dev);

#endif /* BHARATOS_I2C_HID_H */