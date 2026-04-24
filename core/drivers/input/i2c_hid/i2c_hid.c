#include "i2c_hid.h"
#include <string.h>

int i2c_hid_parse_descriptor(i2c_hid_device_t *dev) {
    if (!dev || !dev->i2c) return -1;

    // Send standard request to read the HID descriptor from the device (e.g. register 0x0001)
    uint8_t cmd[2] = {0x01, 0x00}; // Command to read from register 1

    int ret = i2c_master_send(dev->i2c, (const char *)cmd, 2);
    if (ret < 0) return -1;

    // Receive the HID descriptor response
    ret = i2c_master_recv(dev->i2c, (char *)&dev->hid_desc, sizeof(i2c_hid_desc_t));
    if (ret < 0) return -1;

    // Basic structural validation - e.g., the length must be valid per the spec
    if (dev->hid_desc.wHIDDescLength != 0x001E) { // Standard descriptor length is 30 bytes
        return -1; // Malformed descriptor
    }

    return 0;
}

int i2c_hid_probe(i2c_device_t *client, i2c_hid_device_t *out_dev) {
    if (!client || !out_dev) return -1;

    out_dev->i2c = client;
    out_dev->state = 0; // Uninitialized

    // Try parsing the descriptor
    int ret = i2c_hid_parse_descriptor(out_dev);
    if (ret != 0) {
        out_dev->i2c = NULL;
        return -1; // Probe fails
    }

    out_dev->state = 1; // Initialized
    return 0; // Probe succeeds
}

void i2c_hid_remove(i2c_hid_device_t *dev) {
    if (!dev) return;

    dev->state = 0; // Uninitialized
    dev->i2c = NULL;
}