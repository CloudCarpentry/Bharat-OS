#include "i2c_core.h"
#include <stddef.h>

int i2c_transfer(i2c_controller_t *ctrl, i2c_msg_t *msgs, int num) {
    if (!ctrl || !ctrl->ops || !msgs || num <= 0) {
        return -1;
    }

    if (ctrl->ops->master_xfer) {
        return ctrl->ops->master_xfer(ctrl, msgs, num);
    }
    return -1; // Not supported
}

int i2c_master_send(i2c_device_t *client, const char *buf, int count) {
    if (!client || !client->controller || !buf || count < 0) return -1;

    i2c_msg_t msg = {
        .addr = client->addr,
        .flags = 0,
        .len = count,
        .buf = (uint8_t *)buf,
    };

    return i2c_transfer(client->controller, &msg, 1);
}

int i2c_master_recv(i2c_device_t *client, char *buf, int count) {
    if (!client || !client->controller || !buf || count < 0) return -1;

    i2c_msg_t msg = {
        .addr = client->addr,
        .flags = I2C_M_RD,
        .len = count,
        .buf = (uint8_t *)buf,
    };

    return i2c_transfer(client->controller, &msg, 1);
}