#ifndef BHARATOS_I2C_MSG_H
#define BHARATOS_I2C_MSG_H

#include <stdint.h>
#include <stddef.h>

#define I2C_M_RD           0x0001
#define I2C_M_TEN          0x0010
#define I2C_M_RECV_LEN     0x0400
#define I2C_M_NO_RD_ACK    0x0800
#define I2C_M_IGNORE_NAK   0x1000
#define I2C_M_REV_DIR_ADDR 0x2000
#define I2C_M_NOSTART      0x4000
#define I2C_M_STOP         0x8000

typedef struct {
    uint16_t addr;
    uint16_t flags;
    uint16_t len;
    uint8_t *buf;
} i2c_msg_t;

#endif /* BHARATOS_I2C_MSG_H */