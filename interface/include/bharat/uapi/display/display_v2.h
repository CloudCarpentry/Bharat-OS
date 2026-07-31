#ifndef BHARAT_UAPI_DISPLAY_V2_H
#define BHARAT_UAPI_DISPLAY_V2_H

#include <stdint.h>
#include <stdbool.h>
#include "bharat/uapi/display/lease.h"

/* Handle Type Definitions */
typedef uint64_t bh_display_handle_t;
typedef uint64_t bh_display_lease_handle_t;
typedef uint64_t bh_gui_surface_handle_t;
typedef uint64_t bh_gui_buffer_handle_t;
typedef uint64_t bh_gui_fence_handle_t;

/* Monotonic deadline type in nanoseconds */
typedef uint64_t bh_monotonic_deadline_ns_t;

/* Handle Bit Allocation Constants */
#define BH_GUI_HANDLE_SLOT_BITS       16u
#define BH_GUI_HANDLE_GENERATION_BITS 32u
#define BH_GUI_HANDLE_KIND_BITS        8u
#define BH_GUI_HANDLE_VERSION_BITS     8u

#define BH_GUI_HANDLE_SLOT_SHIFT       0u
#define BH_GUI_HANDLE_GENERATION_SHIFT 16u
#define BH_GUI_HANDLE_KIND_SHIFT       48u
#define BH_GUI_HANDLE_VERSION_SHIFT    56u

#define BH_GUI_HANDLE_SLOT_MASK       ((1ULL << BH_GUI_HANDLE_SLOT_BITS) - 1ULL)
#define BH_GUI_HANDLE_GENERATION_MASK ((1ULL << BH_GUI_HANDLE_GENERATION_BITS) - 1ULL)
#define BH_GUI_HANDLE_KIND_MASK       ((1ULL << BH_GUI_HANDLE_KIND_BITS) - 1ULL)
#define BH_GUI_HANDLE_VERSION_MASK    ((1ULL << BH_GUI_HANDLE_VERSION_BITS) - 1ULL)

#define BH_GUI_HANDLE_ABI_V1           1u
#define BH_GUI_HANDLE_INVALID          UINT64_C(0)

/* Resource Kinds */
typedef enum {
    BH_GUI_RESOURCE_INVALID = 0,
    BH_GUI_RESOURCE_DISPLAY = 1,
    BH_GUI_RESOURCE_LEASE   = 2,
    BH_GUI_RESOURCE_SURFACE = 3,
    BH_GUI_RESOURCE_BUFFER  = 4,
    BH_GUI_RESOURCE_FENCE   = 5,
} bh_gui_resource_kind_t;

/* Handle Helpers (Fixed-Width Shift/Mask based) */
static inline uint64_t bh_gui_handle_pack(uint16_t slot, uint32_t generation, uint8_t kind, uint8_t version) {
    return (((uint64_t)version & BH_GUI_HANDLE_VERSION_MASK) << BH_GUI_HANDLE_VERSION_SHIFT) |
           (((uint64_t)kind & BH_GUI_HANDLE_KIND_MASK) << BH_GUI_HANDLE_KIND_SHIFT) |
           (((uint64_t)generation & BH_GUI_HANDLE_GENERATION_MASK) << BH_GUI_HANDLE_GENERATION_SHIFT) |
           (((uint64_t)slot & BH_GUI_HANDLE_SLOT_MASK) << BH_GUI_HANDLE_SLOT_SHIFT);
}

static inline uint16_t bh_gui_handle_get_slot(uint64_t handle) {
    return (uint16_t)((handle >> BH_GUI_HANDLE_SLOT_SHIFT) & BH_GUI_HANDLE_SLOT_MASK);
}

static inline uint32_t bh_gui_handle_get_generation(uint64_t handle) {
    return (uint32_t)((handle >> BH_GUI_HANDLE_GENERATION_SHIFT) & BH_GUI_HANDLE_GENERATION_MASK);
}

static inline uint8_t bh_gui_handle_get_kind(uint64_t handle) {
    return (uint8_t)((handle >> BH_GUI_HANDLE_KIND_SHIFT) & BH_GUI_HANDLE_KIND_MASK);
}

static inline uint8_t bh_gui_handle_get_version(uint64_t handle) {
    return (uint8_t)((handle >> BH_GUI_HANDLE_VERSION_SHIFT) & BH_GUI_HANDLE_VERSION_MASK);
}

static inline bool bh_gui_handle_validate(uint64_t handle, uint8_t expected_kind) {
    if (handle == BH_GUI_HANDLE_INVALID) return false;
    if (bh_gui_handle_get_version(handle) != BH_GUI_HANDLE_ABI_V1) return false;
    if (bh_gui_handle_get_kind(handle) != expected_kind) return false;
    if (bh_gui_handle_get_slot(handle) == 0) return false;
    if (bh_gui_handle_get_generation(handle) == 0) return false;
    return true;
}

/* Domain-Level Service Response Codes */
typedef enum {
    BH_DISPLAY_RESULT_OK = 0,
    BH_DISPLAY_RESULT_INVALID_ARGUMENT = 1,
    BH_DISPLAY_RESULT_BAD_STATE = 2,
    BH_DISPLAY_RESULT_NOT_FOUND = 3,
    BH_DISPLAY_RESULT_STALE_HANDLE = 4,
    BH_DISPLAY_RESULT_WRONG_RESOURCE_TYPE = 5,
    BH_DISPLAY_RESULT_PERMISSION_DENIED = 6,
    BH_DISPLAY_RESULT_PROFILE_UNSUPPORTED = 7,
    BH_DISPLAY_RESULT_FEATURE_DISABLED = 8,
    BH_DISPLAY_RESULT_BUSY = 9,
    BH_DISPLAY_RESULT_TIMEOUT = 10,
    BH_DISPLAY_RESULT_NO_RESOURCES = 11,
    BH_DISPLAY_RESULT_REVOKED = 12,
    BH_DISPLAY_RESULT_IN_PROGRESS = 13,
    BH_DISPLAY_RESULT_DESCRIPTOR_INVALID = 14,
    BH_DISPLAY_RESULT_FORMAT_UNSUPPORTED = 15,
    BH_DISPLAY_RESULT_MODIFIER_UNSUPPORTED = 16,
    BH_DISPLAY_RESULT_MEMORY_DOMAIN_UNSUPPORTED = 17,
    BH_DISPLAY_RESULT_FENCE_UNSIGNALED = 18,
    BH_DISPLAY_RESULT_INTERNAL = 19,
} bh_display_result_t;

/* Buffer v2 State Machine Definitions */
typedef enum {
    BH_BUFFER_STATE_FREE = 0,
    BH_BUFFER_STATE_ALLOCATED = 1,
    BH_BUFFER_STATE_IMPORTED = 2,
    BH_BUFFER_STATE_ATTACHED = 3,
    BH_BUFFER_STATE_QUEUED = 4,
    BH_BUFFER_STATE_SCANNING_OUT = 5,
    BH_BUFFER_STATE_RETIRED = 6,
    BH_BUFFER_STATE_RELEASED = 7,
    BH_BUFFER_STATE_REVOKED = 8,
} bh_buffer_state_v2_t;

/* Software Fence states */
typedef enum {
    BH_FENCE_STATE_FREE = 0,
    BH_FENCE_STATE_UNSIGNALED = 1,
    BH_FENCE_STATE_SIGNALED = 2,
    BH_FENCE_STATE_CANCELLED = 3,
    BH_FENCE_STATE_TIMED_OUT = 4,
    BH_FENCE_STATE_RELEASED = 5,
} bh_fence_state_v2_t;

/* FourCC Pixel Formats */
typedef uint32_t bh_display_pixel_format_t;

#define BH_DISPLAY_FORMAT_INVALID   UINT32_C(0x00000000)
#define BH_DISPLAY_FORMAT_XRGB8888  UINT32_C(0x34325258) /* XR24 */
#define BH_DISPLAY_FORMAT_ARGB8888  UINT32_C(0x34325241) /* AR24 */
#define BH_DISPLAY_FORMAT_RGB565    UINT32_C(0x36314752) /* RG16 */
#define BH_DISPLAY_FORMAT_NV12      UINT32_C(0x3231564E) /* NV12 */

/* Memory Domains */
typedef enum {
    BH_DISPLAY_MEMORY_DOMAIN_INVALID = 0,
    BH_DISPLAY_MEMORY_DOMAIN_SYSTEM = 1,
    BH_DISPLAY_MEMORY_DOMAIN_DMA_CONTIGUOUS = 2,
    BH_DISPLAY_MEMORY_DOMAIN_DEVICE_LOCAL = 3,
    BH_DISPLAY_MEMORY_DOMAIN_PROTECTED = 4,
    BH_DISPLAY_MEMORY_DOMAIN_EXTERNAL_IMPORTED = 5,
} bh_display_memory_domain_t;

/* Usage Flags */
#define BH_DISPLAY_BUFFER_USAGE_CPU_READ       UINT32_C(1u << 0)
#define BH_DISPLAY_BUFFER_USAGE_CPU_WRITE      UINT32_C(1u << 1)
#define BH_DISPLAY_BUFFER_USAGE_COMPOSITOR     UINT32_C(1u << 2)
#define BH_DISPLAY_BUFFER_USAGE_SCANOUT        UINT32_C(1u << 3)
#define BH_DISPLAY_BUFFER_USAGE_CURSOR         UINT32_C(1u << 4)
#define BH_DISPLAY_BUFFER_USAGE_VIDEO          UINT32_C(1u << 5)
#define BH_DISPLAY_BUFFER_USAGE_PROTECTED      UINT32_C(1u << 6)
#define BH_DISPLAY_BUFFER_USAGE_TRUSTED_OVERLAY UINT32_C(1u << 7)

/* Modifiers */
typedef uint64_t bh_display_modifier_t;

#define BH_DISPLAY_MODIFIER_LINEAR  UINT64_C(0)
#define BH_DISPLAY_MODIFIER_INVALID UINT64_MAX

/* Plane Descriptors */
#define BH_DISPLAY_MAX_PLANES 4u

typedef struct {
    uint64_t offset_bytes;
    uint64_t size_bytes;
    uint32_t stride_bytes;
    uint32_t reserved;
} bh_display_buffer_plane_t;

typedef struct {
    uint32_t abi_version;
    uint32_t struct_size;

    uint32_t width;
    uint32_t height;
    uint32_t pixel_format;
    uint32_t usage_flags;

    uint32_t memory_domain;
    uint32_t plane_count;

    uint64_t total_size_bytes;
    uint64_t modifier;

    bh_display_buffer_plane_t planes[BH_DISPLAY_MAX_PLANES];
} bh_display_buffer_desc_t;

/* Direct-Scanout Eligibility and Reasons */
typedef enum {
    BH_SCANOUT_REASON_ELIGIBLE = 0,
    BH_SCANOUT_REASON_MULTIPLE_VISIBLE_SURFACES = 1,
    BH_SCANOUT_REASON_NOT_FULLSCREEN = 2,
    BH_SCANOUT_REASON_FORMAT_UNSUPPORTED = 3,
    BH_SCANOUT_REASON_MODIFIER_UNSUPPORTED = 4,
    BH_SCANOUT_REASON_STRIDE_UNSUPPORTED = 5,
    BH_SCANOUT_REASON_SCALING_UNSUPPORTED = 6,
    BH_SCANOUT_REASON_TRANSFORM_UNSUPPORTED = 7,
    BH_SCANOUT_REASON_MEMORY_DOMAIN_UNSUPPORTED = 8,
    BH_SCANOUT_REASON_USAGE_MISSING = 9,
    BH_SCANOUT_REASON_ACQUIRE_FENCE_UNSIGNALED = 10,
    BH_SCANOUT_REASON_LEASE_INVALID = 11,
    BH_SCANOUT_REASON_RIGHTS_DENIED = 12,
    BH_SCANOUT_REASON_TRUSTED_OVERLAY_CONFLICT = 13,
    BH_SCANOUT_REASON_PROTECTED_CONTENT_CONFLICT = 14,
    BH_SCANOUT_REASON_NO_COMPATIBLE_PLANE = 15,
} bh_direct_scanout_reason_t;

/* Compile-time Assertions to Validate Contract Layout invariants */
#define BH_STATIC_ASSERT(cond, msg) _Static_assert(cond, msg)

BH_STATIC_ASSERT(sizeof(bh_display_buffer_plane_t) == 24, "bh_display_buffer_plane_t must be 24 bytes");
BH_STATIC_ASSERT(sizeof(bh_display_buffer_desc_t) == 144, "bh_display_buffer_desc_t must be 144 bytes");

#endif /* BHARAT_UAPI_DISPLAY_V2_H */
