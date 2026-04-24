#ifndef BHARAT_INPUT_DRIVER_H
#define BHARAT_INPUT_DRIVER_H

#include <stdint.h>
#include <stdbool.h>

struct bharat_input_device;

/**
 * Operations for an input device.
 */
typedef struct {
    int (*open)(struct bharat_input_device *dev);
    void (*close)(struct bharat_input_device *dev);
    int (*flush)(struct bharat_input_device *dev);
} bharat_input_device_ops_t;

/**
 * Input Device Abstraction
 */
typedef struct bharat_input_device {
    const char *name;
    uint32_t id;
    const bharat_input_device_ops_t *ops;
    void *priv_data;

    // Capability bitmasks
    uint32_t evbit[8];   // Supports EV_*
    uint32_t keybit[24]; // Supports KEY_*
    uint32_t relbit[4];  // Supports REL_*
    uint32_t absbit[4];  // Supports ABS_*
} bharat_input_device_t;

// Core API implemented by inputmgr/drivers bridge (IPC/RPC in a real service)
int bharat_input_register(bharat_input_device_t *dev);
void bharat_input_report_event(bharat_input_device_t *dev, uint16_t type, uint16_t code, int32_t value);
void bharat_input_sync(bharat_input_device_t *dev);

#endif // BHARAT_INPUT_DRIVER_H
