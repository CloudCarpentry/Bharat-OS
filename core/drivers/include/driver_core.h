#ifndef BHARAT_DRIVER_CORE_H
#define BHARAT_DRIVER_CORE_H

#include <stdint.h>
#include <stdbool.h>

// ---------------------------------------------------------
// Device Classes
// ---------------------------------------------------------
typedef enum {
    CLASS_UNKNOWN = 0,
    CLASS_GPIO,
    CLASS_PINCTRL,
    CLASS_PWM,
    CLASS_I2C_CTRL,
    CLASS_I2C_DEVICE,
    CLASS_SPI_CTRL,
    CLASS_SPI_DEVICE,
    CLASS_USB_HOST,
    CLASS_USB_DEVICE,
    CLASS_USB_INTERFACE,
    CLASS_LED,
    CLASS_INPUT,
    CLASS_DISPLAY,
    CLASS_SENSOR,
    CLASS_WATCHDOG,
    CLASS_RTC,
    CLASS_POWER,
    CLASS_NET,
    CLASS_STORAGE,
    CLASS_MAX
} device_class_t;

// ---------------------------------------------------------
// Core Descriptors
// ---------------------------------------------------------
struct bus_desc;
struct driver_desc;

typedef struct device_desc {
    const char* name;
    device_class_t dev_class;
    struct bus_desc* bus;
    const char* instance_path;
    const char* compatible_id;
    uint32_t vendor_id;
    uint32_t device_id;

    uint32_t capability_flags;
    uint32_t power_flags;

    struct device_desc* parent;
    // Simplified child list representation for skeleton
    struct device_desc* next_child;
    struct device_desc* first_child;

    void* bus_data;
    void* driver_data;
} device_desc_t;

typedef struct driver_desc {
    const char* name;
    device_class_t supported_class;
    struct bus_desc* supported_bus;
    const char* match_compatible_id;

    int (*probe)(device_desc_t* dev);
    void (*remove)(device_desc_t* dev);
    int (*suspend)(device_desc_t* dev);
    int (*resume)(device_desc_t* dev);
    int (*reset)(device_desc_t* dev);
    void (*fault)(device_desc_t* dev);
} driver_desc_t;

typedef struct bus_desc {
    const char* name;
    int (*register_controller)(device_desc_t* controller);
    int (*enumerate)(device_desc_t* controller);
    int (*register_device)(device_desc_t* dev);
    void (*remove_device)(device_desc_t* dev);
    int (*rescan)(device_desc_t* controller);

    // Optional hooks
    int (*suspend)(struct bus_desc* bus);
    int (*resume)(struct bus_desc* bus);
} bus_desc_t;

// ---------------------------------------------------------
// Events
// ---------------------------------------------------------
typedef enum {
    EVENT_DEVICE_ADDED,
    EVENT_DEVICE_REMOVED,
    EVENT_DEVICE_CHANGED,
    EVENT_DEVICE_SUSPEND,
    EVENT_DEVICE_RESUME,
    EVENT_DEVICE_FAULT
} device_event_type_t;

typedef struct {
    device_event_type_t type;
    device_desc_t* device;
} device_event_t;

#endif // BHARAT_DRIVER_CORE_H