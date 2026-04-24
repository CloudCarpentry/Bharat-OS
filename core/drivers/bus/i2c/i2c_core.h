#ifndef BHARATOS_I2C_CORE_H
#define BHARATOS_I2C_CORE_H

#include "i2c_msg.h"
#include "i2c_device.h"

typedef struct i2c_controller_ops {
    int (*master_xfer)(struct i2c_controller *ctrl, i2c_msg_t *msgs, int num);
    uint32_t (*functionality)(struct i2c_controller *ctrl);
} i2c_controller_ops_t;

typedef struct i2c_controller {
    int bus_id;
    char name[32];
    const i2c_controller_ops_t *ops;
    void *dev_data;
} i2c_controller_t;

int i2c_controller_register(i2c_controller_t *ctrl);
void i2c_controller_unregister(i2c_controller_t *ctrl);

int i2c_transfer(i2c_controller_t *ctrl, i2c_msg_t *msgs, int num);
int i2c_master_send(i2c_device_t *client, const char *buf, int count);
int i2c_master_recv(i2c_device_t *client, char *buf, int count);

#endif /* BHARATOS_I2C_CORE_H */