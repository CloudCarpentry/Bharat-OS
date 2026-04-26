#ifndef BHARAT_DRIVER_CORE_H
#define BHARAT_DRIVER_CORE_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @file driver_core.h
 * @brief Bharat-OS Driver Core canonical descriptors and types.
 *
 * D0 — Driver Registry Contract Hardening:
 * This header defines the stable contract for drivers and devices in Bharat-OS.
 */

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
// Lifecycle States
// ---------------------------------------------------------
typedef enum driver_lifecycle_state {
    DRIVER_STATE_REGISTERED = 0,
    DRIVER_STATE_MATCHED,
    DRIVER_STATE_PROBED,
    DRIVER_STATE_STARTED,
    DRIVER_STATE_STOPPED,
    DRIVER_STATE_REMOVED,
    DRIVER_STATE_FAILED,
} driver_lifecycle_state_t;

// ---------------------------------------------------------
// Core Descriptors
// ---------------------------------------------------------
struct bus_desc;
struct driver_desc;

/**
 * @brief device_desc_t is the canonical Bharat-OS device descriptor.
 * It describes the identity and hardware properties of a device.
 */
typedef struct device_desc {
    const char* name;
    uint32_t device_registry_id; /**< Registry-assigned unique handle (dynamic) */
    device_class_t dev_class;
    struct bus_desc* bus;
    const char* instance_path;
    const char* compatible_id;
    uint32_t vendor_id;
    uint32_t device_id;          /**< Hardware-reported device ID */
    uint32_t hw_device_id;       /**< Optional hardware identity (e.g. PCI ID, FDT handle) */

    uint32_t capability_flags;
    uint32_t power_flags;

    struct device_desc* parent;
    // Simplified child list representation for skeleton
    struct device_desc* next_child;
    struct device_desc* first_child;

    void* bus_data;
    void* driver_data;           /**< Deprecated: use device_binding_t for bound state */
} device_desc_t;

/**
 * @brief driver_desc_t is the canonical Bharat-OS driver descriptor.
 * It describes the driver's capabilities and lifecycle operations.
 */
typedef struct driver_desc {
    const char* name;
    uint32_t driver_registry_id; /**< Registry-assigned unique handle (dynamic) */
    device_class_t supported_class;
    struct bus_desc* supported_bus;
    const char* match_compatible_id;

    /* Match scoring extensions */
    uint32_t match_vendor_id;
    uint32_t match_device_id;
    device_class_t match_class;
    uint32_t match_flags;
    int32_t priority;

    int (*probe)(device_desc_t* dev);
    void (*remove)(device_desc_t* dev);
    int (*suspend)(device_desc_t* dev);
    int (*resume)(device_desc_t* dev);
    int (*reset)(device_desc_t* dev);
    void (*fault)(device_desc_t* dev);
} driver_desc_t;

/**
 * @brief device_binding_t is the canonical relationship/lifecycle object.
 * It tracks the active binding between a device and a driver.
 */
typedef struct device_binding {
    uint32_t binding_id;
    device_desc_t *device;
    driver_desc_t *driver;
    driver_lifecycle_state_t state;
    int match_score;
    uint32_t match_priority;
    void *driver_ctx;
} device_binding_t;

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

/* Match wildcards */
#define BH_DRIVER_MATCH_ANY_U16 0xFFFF
#define BH_DRIVER_MATCH_ANY_U32 0xFFFFFFFFu

#endif // BHARAT_DRIVER_CORE_H
