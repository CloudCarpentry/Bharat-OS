#ifndef BHARATOS_I2C_HID_DESC_H
#define BHARATOS_I2C_HID_DESC_H

#include <stdint.h>

// I2C HID Descriptor structure per specification
typedef struct __attribute__((packed)) {
    uint16_t wHIDDescLength;
    uint16_t bcdVersion;
    uint16_t wReportDescLength;
    uint16_t wReportDescRegister;
    uint16_t wInputRegister;
    uint16_t wMaxInputLength;
    uint16_t wOutputRegister;
    uint16_t wMaxOutputLength;
    uint16_t wCommandRegister;
    uint16_t wDataRegister;
    uint16_t wVendorID;
    uint16_t wProductID;
    uint16_t wVersionID;
    uint32_t reserved;
} i2c_hid_desc_t;

#endif /* BHARATOS_I2C_HID_DESC_H */