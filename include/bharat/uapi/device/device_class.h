#ifndef BHARAT_UAPI_DEVICE_CLASS_H
#define BHARAT_UAPI_DEVICE_CLASS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Normalized Device Class Registry
 * Defines system-wide classes for logical devices.
 * This is a shared contract for classification, queries, and descriptors.
 */
typedef enum {
    BHARAT_DEV_CLASS_UNKNOWN = 0,
    BHARAT_DEV_CLASS_DISPLAY,
    BHARAT_DEV_CLASS_INPUT,
    BHARAT_DEV_CLASS_NET,
    BHARAT_DEV_CLASS_SENSOR,
    BHARAT_DEV_CLASS_STORAGE,
    BHARAT_DEV_CLASS_AUDIO,
    BHARAT_DEV_CLASS_CAMERA,
    BHARAT_DEV_CLASS_BATTERY,
    BHARAT_DEV_CLASS_MOTOR_ACTUATOR,
    BHARAT_DEV_CLASS_WATCHDOG_SAFETY,
    BHARAT_DEV_CLASS_RADIO,
    BHARAT_DEV_CLASS_MAX
} bharat_device_class_t;

/**
 * Common Device Attributes Flags
 */
#define BHARAT_DEV_ATTR_REMOVABLE  (1ull << 0)
#define BHARAT_DEV_ATTR_POWER_MGT  (1ull << 1)
#define BHARAT_DEV_ATTR_SECURE     (1ull << 2)

/**
 * Normalized Device Descriptor
 * Represents a device exported to services or queried from drivers.
 */
typedef struct {
    uint32_t device_id;
    bharat_device_class_t dev_class;
    uint32_t vendor_id;
    uint32_t product_id;
    uint64_t attributes;
    char name[32];
} bharat_device_descriptor_t;

/**
 * Standard Device Query Metadata Shape
 * Services can request subsets of this data when querying standard devices.
 */
typedef struct {
    bharat_device_class_t dev_class;
    uint32_t max_results;
    uint32_t capabilities_mask;
} bharat_device_query_req_t;

typedef struct {
    uint32_t result_count;
    bharat_device_descriptor_t results[8]; /* Bound array for standard IPC replies */
} bharat_device_query_res_t;

#ifdef __cplusplus
}
#endif

#endif /* BHARAT_UAPI_DEVICE_CLASS_H */
