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
 * Canonical display subclasses for UI/media stacks.
 */
typedef enum {
    BHARAT_DISPLAY_SUBCLASS_UNKNOWN = 0,
    BHARAT_DISPLAY_SUBCLASS_PANEL,
    BHARAT_DISPLAY_SUBCLASS_FRAMEBUFFER,
    BHARAT_DISPLAY_SUBCLASS_GPU,
    BHARAT_DISPLAY_SUBCLASS_VIDEO_OUT
} bharat_display_subclass_t;

/**
 * Canonical input subclasses for UI/HMI stacks.
 */
typedef enum {
    BHARAT_INPUT_SUBCLASS_UNKNOWN = 0,
    BHARAT_INPUT_SUBCLASS_TOUCH,
    BHARAT_INPUT_SUBCLASS_KEYPAD,
    BHARAT_INPUT_SUBCLASS_ROTARY,
    BHARAT_INPUT_SUBCLASS_IR,
    BHARAT_INPUT_SUBCLASS_CAN_CONTROL
} bharat_input_subclass_t;

/**
 * Common Device Attributes Flags
 */
#define BHARAT_DEV_ATTR_REMOVABLE  (1ull << 0)
#define BHARAT_DEV_ATTR_POWER_MGT  (1ull << 1)
#define BHARAT_DEV_ATTR_SECURE     (1ull << 2)

/**
 * Display capability bits used by generic service queries.
 */
#define BHARAT_DISPLAY_CAP_OVERLAY            (1ull << 0)
#define BHARAT_DISPLAY_CAP_DIRECT_SCANOUT     (1ull << 1)
#define BHARAT_DISPLAY_CAP_SECURE_OVERLAY     (1ull << 2)
#define BHARAT_DISPLAY_CAP_MODESET            (1ull << 3)
#define BHARAT_DISPLAY_CAP_VSYNC_SIGNAL       (1ull << 4)

/**
 * Input capability bits used by generic service queries.
 */
#define BHARAT_INPUT_CAP_ABSOLUTE_COORDS      (1ull << 0)
#define BHARAT_INPUT_CAP_RELATIVE_COORDS      (1ull << 1)
#define BHARAT_INPUT_CAP_KEYS                 (1ull << 2)
#define BHARAT_INPUT_CAP_ROTARY_STEPS         (1ull << 3)
#define BHARAT_INPUT_CAP_IR_CODES             (1ull << 4)
#define BHARAT_INPUT_CAP_CAN_SIGNALS          (1ull << 5)

/**
 * Normalized Device Descriptor
 * Represents a device exported to services or queried from drivers.
 */
typedef struct {
    uint32_t device_id;
    bharat_device_class_t dev_class;
    uint32_t subclass;
    uint32_t vendor_id;
    uint32_t product_id;
    uint64_t attributes;
    char name[32];
} bharat_device_descriptor_t;

typedef struct {
    uint32_t min_hz;
    uint32_t max_hz;
} bharat_refresh_range_t;

typedef struct {
    uint32_t width_px;
    uint32_t height_px;
    uint32_t min_stride_bytes;
} bharat_display_mode_caps_t;

typedef struct {
    uint32_t plane_count;
    uint32_t pixel_format_count;
    uint64_t capability_bits;
    bharat_refresh_range_t refresh_hz;
    bharat_display_mode_caps_t max_mode;
} bharat_display_caps_t;

typedef struct {
    uint32_t source_id;
    uint32_t coordinate_mode;
    uint32_t keymap_set_id;
    uint32_t rotary_resolution;
    uint64_t capability_bits;
} bharat_input_caps_t;

typedef struct {
    uint64_t timestamp_ns;
    uint16_t type;
    uint16_t code;
    int32_t value;
} bharat_input_event_wire_t;

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
