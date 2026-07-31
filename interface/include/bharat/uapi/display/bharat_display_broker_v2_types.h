#pragma once
#include <stdint.h>

typedef struct {
    uint32_t dummy;
} bharat_display_broker_v2_EnumerateDisplaysReq_t;

typedef struct {
    uint32_t result;
    uint32_t display_count;
    uint64_t display_handle_0;
    uint64_t display_handle_1;
    uint64_t display_handle_2;
    uint64_t display_handle_3;
} bharat_display_broker_v2_EnumerateDisplaysResp_t;

typedef struct {
    uint64_t display_handle;
} bharat_display_broker_v2_QueryDisplayModeReq_t;

typedef struct {
    uint32_t result;
    uint32_t width;
    uint32_t height;
    uint32_t refresh_hz;
    uint32_t pixel_format;
    uint32_t flags;
} bharat_display_broker_v2_QueryDisplayModeResp_t;

typedef struct {
    uint64_t display_handle;
    uint32_t plane_id;
    uint32_t reserved;
} bharat_display_broker_v2_QueryPlaneCapabilitiesReq_t;

typedef struct {
    uint32_t result;
    uint32_t supported_formats_mask;
    uint32_t min_width;
    uint32_t min_height;
    uint32_t max_width;
    uint32_t max_height;
    uint32_t scaling_support;
    uint32_t rotation_support;
    uint32_t secure_overlay_support;
    uint32_t cursor_plane_support;
    uint32_t direct_scanout_support;
    uint32_t z_order_min;
    uint32_t z_order_max;
} bharat_display_broker_v2_QueryPlaneCapabilitiesResp_t;

typedef struct {
    uint64_t display_handle;
    uint32_t requested_rights;
    uint32_t reserved;
} bharat_display_broker_v2_RequestDisplayLeaseReq_t;

typedef struct {
    uint32_t result;
    uint32_t granted_rights;
    uint64_t lease_handle;
} bharat_display_broker_v2_RequestDisplayLeaseResp_t;

typedef struct {
    uint64_t lease_handle;
} bharat_display_broker_v2_AcknowledgeLeaseRevocationReq_t;

typedef struct {
    uint32_t result;
    uint32_t reserved;
} bharat_display_broker_v2_AcknowledgeLeaseRevocationResp_t;

typedef struct {
    uint64_t lease_handle;
} bharat_display_broker_v2_ReleaseDisplayLeaseReq_t;

typedef struct {
    uint32_t result;
    uint32_t reserved;
} bharat_display_broker_v2_ReleaseDisplayLeaseResp_t;

typedef struct {
    uint64_t lease_handle;
    uint32_t width;
    uint32_t height;
    uint32_t z_order;
    uint32_t reserved;
} bharat_display_broker_v2_CreateSurfaceReq_t;

typedef struct {
    uint32_t result;
    uint32_t reserved;
    uint64_t surface_handle;
} bharat_display_broker_v2_CreateSurfaceResp_t;

typedef struct {
    uint64_t lease_handle;
    uint64_t surface_handle;
} bharat_display_broker_v2_DestroySurfaceReq_t;

typedef struct {
    uint32_t result;
    uint32_t reserved;
} bharat_display_broker_v2_DestroySurfaceResp_t;

typedef struct {
    uint64_t lease_handle;
    uint32_t width;
    uint32_t height;
    uint32_t pixel_format;
    uint32_t usage_flags;
    uint32_t memory_domain;
    uint32_t plane_count;
    uint64_t total_size_bytes;
    uint64_t modifier;
    uint64_t plane0_offset_bytes;
    uint64_t plane0_size_bytes;
    uint32_t plane0_stride_bytes;
    uint32_t plane0_reserved;
    uint64_t plane1_offset_bytes;
    uint64_t plane1_size_bytes;
    uint32_t plane1_stride_bytes;
    uint32_t plane1_reserved;
    uint64_t plane2_offset_bytes;
    uint64_t plane2_size_bytes;
    uint32_t plane2_stride_bytes;
    uint32_t plane2_reserved;
    uint64_t plane3_offset_bytes;
    uint64_t plane3_size_bytes;
    uint32_t plane3_stride_bytes;
    uint32_t plane3_reserved;
} bharat_display_broker_v2_RegisterBufferReq_t;

typedef struct {
    uint32_t result;
    uint32_t reserved;
    uint64_t buffer_handle;
} bharat_display_broker_v2_RegisterBufferResp_t;

typedef struct {
    uint64_t lease_handle;
    uint64_t buffer_handle;
} bharat_display_broker_v2_ReleaseBufferReq_t;

typedef struct {
    uint32_t result;
    uint32_t reserved;
} bharat_display_broker_v2_ReleaseBufferResp_t;

typedef struct {
    uint64_t lease_handle;
    uint64_t surface_handle;
    uint64_t buffer_handle;
} bharat_display_broker_v2_AttachBufferReq_t;

typedef struct {
    uint32_t result;
    uint32_t reserved;
} bharat_display_broker_v2_AttachBufferResp_t;

typedef struct {
    uint64_t lease_handle;
    uint64_t surface_handle;
    uint64_t buffer_handle;
    uint64_t acquire_fence_handle;
    uint64_t deadline_ns;
} bharat_display_broker_v2_PresentSurfaceReq_t;

typedef struct {
    uint32_t result;
    uint32_t direct_scanout_reason;
    uint64_t release_fence_handle;
} bharat_display_broker_v2_PresentSurfaceResp_t;

typedef struct {
    uint64_t lease_handle;
    uint64_t surface_handle;
} bharat_display_broker_v2_RetirePresentationReq_t;

typedef struct {
    uint32_t result;
    uint32_t reserved;
} bharat_display_broker_v2_RetirePresentationResp_t;

typedef struct {
    uint64_t lease_handle;
    uint64_t surface_handle;
} bharat_display_broker_v2_QueryPresentationStatusReq_t;

typedef struct {
    uint32_t result;
    uint32_t presentation_state;
    uint64_t active_buffer_handle;
    uint64_t frame_counter;
} bharat_display_broker_v2_QueryPresentationStatusResp_t;
