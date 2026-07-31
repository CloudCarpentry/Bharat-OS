#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "bharat/uapi/display/lease.h"
#include "bharat/uapi/display/display.h"
#include "bharat/uapi/display/display_v2.h"
#include "bharat/uapi/display/bharat_display_broker_v2_types.h"
#include "bharat/uapi/gui/surface_v2.h"
#include "bharat/uapi/ipc/status.h"
#include "bharat/uapi/ipc/manifest.h"
#include "bharat/runtime/runtime.h"
#include <bharat/service/service_runtime.h>
#include <bharat/ipc/ipc.h>

#include "../../../stacks/media/compositor/compositor_core.h"

/**
 * Bharat-OS Display Broker Service (V2 Refactored)
 */

#define FB_WIDTH 800
#define FB_HEIGHT 480
#define FB_SIZE_PX (FB_WIDTH * FB_HEIGHT)

static bh_compositor_core_t g_core;
static bh_display_handle_t g_default_display_0 = BH_GUI_HANDLE_INVALID;
static bh_display_handle_t g_default_display_1 = BH_GUI_HANDLE_INVALID;

static uint64_t sys_clock_now(void *ctx) {
    (void)ctx;
    /* Static simple tick counter representing nanoseconds */
    static uint64_t time_ns = 1000000;
    time_ns += 16666666ULL; /* ~60Hz ticks */
    return time_ns;
}

static void broker_init(void) {
    bh_profile_caps_t caps = {
        .profile_class = BH_PROFILE_CLASS_DESKTOP,
        .ui_enabled = true,
        .compositor_enabled = true,
        .direct_scanout_allowed = true,
        .trusted_overlay_allowed = true
    };
    bh_gui_clock_t clock = {
        .now_ns = sys_clock_now,
        .ctx = NULL
    };
    bh_compositor_core_init(&g_core, &caps, &clock);

    /* Register Display 0 */
    bh_compositor_add_display(&g_core, FB_WIDTH, FB_HEIGHT, 60, BH_DISPLAY_FORMAT_XRGB8888, &g_default_display_0);

    /* Setup 4 display planes on Display 0 */
    bh_plane_desc_t plane0 = {
        .supported_formats_mask = 0xF,
        .min_width = 1,
        .min_height = 1,
        .max_width = 4096,
        .max_height = 4096,
        .scaling_support = false,
        .rotation_support = false,
        .secure_overlay_support = false,
        .cursor_plane_support = false,
        .direct_scanout_support = true,
        .z_order_min = 0,
        .z_order_max = 5
    };
    bh_compositor_add_display_plane(&g_core, g_default_display_0, &plane0);

    bh_plane_desc_t plane1 = {
        .supported_formats_mask = 0xF,
        .min_width = 1,
        .min_height = 1,
        .max_width = 4096,
        .max_height = 4096,
        .scaling_support = false,
        .rotation_support = false,
        .secure_overlay_support = false,
        .cursor_plane_support = false,
        .direct_scanout_support = true,
        .z_order_min = 1,
        .z_order_max = 5
    };
    bh_compositor_add_display_plane(&g_core, g_default_display_0, &plane1);

    bh_plane_desc_t plane2 = {
        .supported_formats_mask = 0xF,
        .min_width = 1,
        .min_height = 1,
        .max_width = 256,
        .max_height = 256,
        .scaling_support = false,
        .rotation_support = false,
        .secure_overlay_support = false,
        .cursor_plane_support = true,
        .direct_scanout_support = true,
        .z_order_min = 4,
        .z_order_max = 5
    };
    bh_compositor_add_display_plane(&g_core, g_default_display_0, &plane2);

    bh_plane_desc_t plane3 = {
        .supported_formats_mask = 0xF,
        .min_width = 1,
        .min_height = 1,
        .max_width = 4096,
        .max_height = 4096,
        .scaling_support = false,
        .rotation_support = false,
        .secure_overlay_support = true,
        .cursor_plane_support = false,
        .direct_scanout_support = true,
        .z_order_min = 5,
        .z_order_max = 5
    };
    bh_compositor_add_display_plane(&g_core, g_default_display_0, &plane3);

    /* Register Optional Display 1 */
    bh_compositor_add_display(&g_core, 1920, 1080, 60, BH_DISPLAY_FORMAT_XRGB8888, &g_default_display_1);
    bh_compositor_add_display_plane(&g_core, g_default_display_1, &plane0);
}

static void send_reply(bharat_cap_handle_t target, const bharat_ipc_msg_header_t *orig_hdr, bharat_status_t status, void *payload, uint32_t payload_size) {
    bharat_ipc_msg_header_t reply_hdr = *orig_hdr;
    reply_hdr.flags |= BHARAT_IPC_FLAG_REPLY;
    reply_hdr.status = status;
    reply_hdr.payload_size = payload_size;
    bharat_ipc_send(target, &reply_hdr, payload);
}

/* -------------------------------------------------------------
 * V1 Backwards Compatibility Handlers (Translates to V2 under the hood)
 * ------------------------------------------------------------- */

static void handle_request_lease_v1(const bharat_ipc_msg_header_t *hdr, const void *payload) {
    if (hdr->payload_size < 8) {
        send_reply(hdr->reply_endpoint, hdr, BHARAT_IPC_STATUS_ERR_LENGTH, NULL, 0);
        return;
    }

    const struct { uint32_t display_id; uint32_t requested_rights; } *req = payload;
    struct { uint32_t status; uint32_t lease_id; uint32_t granted_rights; uint64_t fb_ptr; } resp;

    bh_display_handle_t disp = (req->display_id == 2) ? g_default_display_1 : g_default_display_0;
    bh_display_lease_handle_t lease_h;
    bh_display_result_t res = bh_compositor_request_lease(&g_core, disp, req->requested_rights, 100, (uint64_t)hdr->reply_endpoint, &lease_h);

    if (res == BH_DISPLAY_RESULT_OK) {
        resp.status = BHARAT_IPC_STATUS_OK;
        resp.lease_id = bh_gui_handle_get_slot(lease_h);
        resp.granted_rights = req->requested_rights;
        resp.fb_ptr = 0x10000000ULL; /* Mock linear address for V1 client compatibility */
        send_reply(hdr->reply_endpoint, hdr, BHARAT_STATUS_OK, &resp, sizeof(resp));
    } else {
        resp.status = BHARAT_IPC_STATUS_ERR_INTERNAL;
        send_reply(hdr->reply_endpoint, hdr, BHARAT_STATUS_ERR_INTERNAL, &resp, sizeof(resp));
    }
}

static void handle_release_lease_v1(const bharat_ipc_msg_header_t *hdr, const void *payload) {
    if (hdr->payload_size < 4) {
        send_reply(hdr->reply_endpoint, hdr, BHARAT_IPC_STATUS_ERR_LENGTH, NULL, 0);
        return;
    }
    const uint32_t *lease_id = payload;
    bh_display_lease_handle_t lease_h = bh_gui_handle_pack(*lease_id, g_core.leases[*lease_id - 1].generation, BH_GUI_RESOURCE_LEASE, BH_GUI_HANDLE_ABI_V1);

    bh_display_result_t res = bh_compositor_release_lease(&g_core, lease_h);
    uint32_t resp_status = (res == BH_DISPLAY_RESULT_OK) ? BHARAT_STATUS_OK : BHARAT_IPC_STATUS_ERR_INTERNAL;
    send_reply(hdr->reply_endpoint, hdr, resp_status, NULL, 0);
}

static void handle_present_v1(const bharat_ipc_msg_header_t *hdr, const void *payload) {
    if (hdr->payload_size < 8) {
        send_reply(hdr->reply_endpoint, hdr, BHARAT_IPC_STATUS_ERR_LENGTH, NULL, 0);
        return;
    }

    const struct { uint32_t lease_id; uint32_t buffer_handle; } *req = payload;

    bh_display_lease_handle_t lease_h = bh_gui_handle_pack(req->lease_id, g_core.leases[req->lease_id - 1].generation, BH_GUI_RESOURCE_LEASE, BH_GUI_HANDLE_ABI_V1);
    if (!bh_gui_handle_validate(lease_h, BH_GUI_RESOURCE_LEASE)) {
        send_reply(hdr->reply_endpoint, hdr, BHARAT_IPC_STATUS_ERR_PERM, NULL, 0);
        return;
    }

    send_reply(hdr->reply_endpoint, hdr, BHARAT_STATUS_OK, NULL, 0);
}

/* -------------------------------------------------------------
 * V2 IPC Method Handlers
 * ------------------------------------------------------------- */

static void handle_enumerate_displays_v2(const bharat_ipc_msg_header_t *hdr, const void *payload) {
    (void)payload;
    bharat_display_broker_v2_EnumerateDisplaysResp_t resp = {0};

    if (hdr->payload_size < sizeof(bharat_display_broker_v2_EnumerateDisplaysReq_t)) {
        send_reply(hdr->reply_endpoint, hdr, BHARAT_IPC_STATUS_ERR_LENGTH, NULL, 0);
        return;
    }

    uint32_t count = 0;
    if (g_core.displays[0].active) {
        resp.display_handle_0 = g_core.displays[0].handle;
        count++;
    }
    if (g_core.displays[1].active) {
        resp.display_handle_1 = g_core.displays[1].handle;
        count++;
    }
    resp.result = BH_DISPLAY_RESULT_OK;
    resp.display_count = count;

    send_reply(hdr->reply_endpoint, hdr, BHARAT_STATUS_OK, &resp, sizeof(resp));
}

static void handle_query_display_mode_v2(const bharat_ipc_msg_header_t *hdr, const void *payload) {
    const bharat_display_broker_v2_QueryDisplayModeReq_t *req = payload;
    bharat_display_broker_v2_QueryDisplayModeResp_t resp = {0};

    if (hdr->payload_size < sizeof(*req)) {
        send_reply(hdr->reply_endpoint, hdr, BHARAT_IPC_STATUS_ERR_LENGTH, NULL, 0);
        return;
    }

    if (!bh_gui_handle_validate(req->display_handle, BH_GUI_RESOURCE_DISPLAY)) {
        resp.result = BH_DISPLAY_RESULT_INVALID_ARGUMENT;
        send_reply(hdr->reply_endpoint, hdr, BHARAT_STATUS_OK, &resp, sizeof(resp));
        return;
    }

    uint16_t slot = bh_gui_handle_get_slot(req->display_handle);
    uint32_t idx = slot - 1;
    if (idx >= MAX_DISPLAYS || !g_core.displays[idx].active || g_core.displays[idx].handle != req->display_handle) {
        resp.result = BH_DISPLAY_RESULT_NOT_FOUND;
    } else {
        resp.result = BH_DISPLAY_RESULT_OK;
        resp.width = g_core.displays[idx].width;
        resp.height = g_core.displays[idx].height;
        resp.refresh_hz = g_core.displays[idx].refresh_hz;
        resp.pixel_format = g_core.displays[idx].pixel_format;
        resp.flags = g_core.displays[idx].flags;
    }

    send_reply(hdr->reply_endpoint, hdr, BHARAT_STATUS_OK, &resp, sizeof(resp));
}

static void handle_query_plane_capabilities_v2(const bharat_ipc_msg_header_t *hdr, const void *payload) {
    const bharat_display_broker_v2_QueryPlaneCapabilitiesReq_t *req = payload;
    bharat_display_broker_v2_QueryPlaneCapabilitiesResp_t resp = {0};

    if (hdr->payload_size < sizeof(*req)) {
        send_reply(hdr->reply_endpoint, hdr, BHARAT_IPC_STATUS_ERR_LENGTH, NULL, 0);
        return;
    }

    if (!bh_gui_handle_validate(req->display_handle, BH_GUI_RESOURCE_DISPLAY)) {
        resp.result = BH_DISPLAY_RESULT_INVALID_ARGUMENT;
        send_reply(hdr->reply_endpoint, hdr, BHARAT_STATUS_OK, &resp, sizeof(resp));
        return;
    }

    uint16_t slot = bh_gui_handle_get_slot(req->display_handle);
    uint32_t idx = slot - 1;
    if (idx >= MAX_DISPLAYS || !g_core.displays[idx].active || g_core.displays[idx].handle != req->display_handle) {
        resp.result = BH_DISPLAY_RESULT_NOT_FOUND;
        send_reply(hdr->reply_endpoint, hdr, BHARAT_STATUS_OK, &resp, sizeof(resp));
        return;
    }

    const bh_display_entry_t *disp = &g_core.displays[idx];
    if (req->plane_id >= disp->plane_count) {
        resp.result = BH_DISPLAY_RESULT_NOT_FOUND;
    } else {
        const bh_plane_desc_t *p = &disp->planes[req->plane_id];
        resp.result = BH_DISPLAY_RESULT_OK;
        resp.supported_formats_mask = p->supported_formats_mask;
        resp.min_width = p->min_width;
        resp.min_height = p->min_height;
        resp.max_width = p->max_width;
        resp.max_height = p->max_height;
        resp.scaling_support = p->scaling_support;
        resp.rotation_support = p->rotation_support;
        resp.secure_overlay_support = p->secure_overlay_support;
        resp.cursor_plane_support = p->cursor_plane_support;
        resp.direct_scanout_support = p->direct_scanout_support;
        resp.z_order_min = p->z_order_min;
        resp.z_order_max = p->z_order_max;
    }

    send_reply(hdr->reply_endpoint, hdr, BHARAT_STATUS_OK, &resp, sizeof(resp));
}

static void handle_request_display_lease_v2(const bharat_ipc_msg_header_t *hdr, const void *payload) {
    const bharat_display_broker_v2_RequestDisplayLeaseReq_t *req = payload;
    bharat_display_broker_v2_RequestDisplayLeaseResp_t resp = {0};

    if (hdr->payload_size < sizeof(*req)) {
        send_reply(hdr->reply_endpoint, hdr, BHARAT_IPC_STATUS_ERR_LENGTH, NULL, 0);
        return;
    }

    bh_display_lease_handle_t lease_h;
    bh_display_result_t res = bh_compositor_request_lease(&g_core, req->display_handle, req->requested_rights, 100, (uint64_t)hdr->reply_endpoint, &lease_h);

    resp.result = res;
    if (res == BH_DISPLAY_RESULT_OK) {
        resp.granted_rights = req->requested_rights;
        resp.lease_handle = lease_h;
    }

    send_reply(hdr->reply_endpoint, hdr, BHARAT_STATUS_OK, &resp, sizeof(resp));
}

static void handle_acknowledge_lease_revocation_v2(const bharat_ipc_msg_header_t *hdr, const void *payload) {
    const bharat_display_broker_v2_AcknowledgeLeaseRevocationReq_t *req = payload;
    bharat_display_broker_v2_AcknowledgeLeaseRevocationResp_t resp = {0};

    if (hdr->payload_size < sizeof(*req)) {
        send_reply(hdr->reply_endpoint, hdr, BHARAT_IPC_STATUS_ERR_LENGTH, NULL, 0);
        return;
    }

    resp.result = bh_compositor_acknowledge_lease_revocation(&g_core, req->lease_handle);
    send_reply(hdr->reply_endpoint, hdr, BHARAT_STATUS_OK, &resp, sizeof(resp));
}

static void handle_release_display_lease_v2(const bharat_ipc_msg_header_t *hdr, const void *payload) {
    const bharat_display_broker_v2_ReleaseDisplayLeaseReq_t *req = payload;
    bharat_display_broker_v2_ReleaseDisplayLeaseResp_t resp = {0};

    if (hdr->payload_size < sizeof(*req)) {
        send_reply(hdr->reply_endpoint, hdr, BHARAT_IPC_STATUS_ERR_LENGTH, NULL, 0);
        return;
    }

    resp.result = bh_compositor_release_lease(&g_core, req->lease_handle);
    send_reply(hdr->reply_endpoint, hdr, BHARAT_STATUS_OK, &resp, sizeof(resp));
}

static void handle_create_surface_v2(const bharat_ipc_msg_header_t *hdr, const void *payload) {
    const bharat_display_broker_v2_CreateSurfaceReq_t *req = payload;
    bharat_display_broker_v2_CreateSurfaceResp_t resp = {0};

    if (hdr->payload_size < sizeof(*req)) {
        send_reply(hdr->reply_endpoint, hdr, BHARAT_IPC_STATUS_ERR_LENGTH, NULL, 0);
        return;
    }

    bh_gui_surface_handle_t surf_h;
    resp.result = bh_compositor_create_surface(&g_core, req->lease_handle, req->width, req->height, req->z_order, &surf_h);
    if (resp.result == BH_DISPLAY_RESULT_OK) {
        resp.surface_handle = surf_h;
    }

    send_reply(hdr->reply_endpoint, hdr, BHARAT_STATUS_OK, &resp, sizeof(resp));
}

static void handle_destroy_surface_v2(const bharat_ipc_msg_header_t *hdr, const void *payload) {
    const bharat_display_broker_v2_DestroySurfaceReq_t *req = payload;
    bharat_display_broker_v2_DestroySurfaceResp_t resp = {0};

    if (hdr->payload_size < sizeof(*req)) {
        send_reply(hdr->reply_endpoint, hdr, BHARAT_IPC_STATUS_ERR_LENGTH, NULL, 0);
        return;
    }

    resp.result = bh_compositor_destroy_surface(&g_core, req->lease_handle, req->surface_handle);
    send_reply(hdr->reply_endpoint, hdr, BHARAT_STATUS_OK, &resp, sizeof(resp));
}

static void handle_register_buffer_v2(const bharat_ipc_msg_header_t *hdr, const void *payload) {
    const bharat_display_broker_v2_RegisterBufferReq_t *req = payload;
    bharat_display_broker_v2_RegisterBufferResp_t resp = {0};

    if (hdr->payload_size < sizeof(*req)) {
        send_reply(hdr->reply_endpoint, hdr, BHARAT_IPC_STATUS_ERR_LENGTH, NULL, 0);
        return;
    }

    bh_display_buffer_desc_t desc = {
        .abi_version = BH_GUI_HANDLE_ABI_V1,
        .struct_size = sizeof(desc),
        .width = req->width,
        .height = req->height,
        .pixel_format = req->pixel_format,
        .usage_flags = req->usage_flags,
        .memory_domain = req->memory_domain,
        .plane_count = req->plane_count,
        .total_size_bytes = req->total_size_bytes,
        .modifier = req->modifier,
        .planes = {
            { .offset_bytes = req->plane0_offset_bytes, .size_bytes = req->plane0_size_bytes, .stride_bytes = req->plane0_stride_bytes },
            { .offset_bytes = req->plane1_offset_bytes, .size_bytes = req->plane1_size_bytes, .stride_bytes = req->plane1_stride_bytes },
            { .offset_bytes = req->plane2_offset_bytes, .size_bytes = req->plane2_size_bytes, .stride_bytes = req->plane2_stride_bytes },
            { .offset_bytes = req->plane3_offset_bytes, .size_bytes = req->plane3_size_bytes, .stride_bytes = req->plane3_stride_bytes }
        }
    };

    bh_gui_buffer_handle_t buf_h;
    resp.result = bh_compositor_register_buffer(&g_core, req->lease_handle, &desc, &buf_h);
    if (resp.result == BH_DISPLAY_RESULT_OK) {
        resp.buffer_handle = buf_h;
    }

    send_reply(hdr->reply_endpoint, hdr, BHARAT_STATUS_OK, &resp, sizeof(resp));
}

static void handle_release_buffer_v2(const bharat_ipc_msg_header_t *hdr, const void *payload) {
    const bharat_display_broker_v2_ReleaseBufferReq_t *req = payload;
    bharat_display_broker_v2_ReleaseBufferResp_t resp = {0};

    if (hdr->payload_size < sizeof(*req)) {
        send_reply(hdr->reply_endpoint, hdr, BHARAT_IPC_STATUS_ERR_LENGTH, NULL, 0);
        return;
    }

    resp.result = bh_compositor_release_buffer(&g_core, req->lease_handle, req->buffer_handle);
    send_reply(hdr->reply_endpoint, hdr, BHARAT_STATUS_OK, &resp, sizeof(resp));
}

static void handle_attach_buffer_v2(const bharat_ipc_msg_header_t *hdr, const void *payload) {
    const bharat_display_broker_v2_AttachBufferReq_t *req = payload;
    bharat_display_broker_v2_AttachBufferResp_t resp = {0};

    if (hdr->payload_size < sizeof(*req)) {
        send_reply(hdr->reply_endpoint, hdr, BHARAT_IPC_STATUS_ERR_LENGTH, NULL, 0);
        return;
    }

    resp.result = bh_compositor_attach_buffer(&g_core, req->lease_handle, req->surface_handle, req->buffer_handle);
    send_reply(hdr->reply_endpoint, hdr, BHARAT_STATUS_OK, &resp, sizeof(resp));
}

static void handle_present_surface_v2(const bharat_ipc_msg_header_t *hdr, const void *payload) {
    const bharat_display_broker_v2_PresentSurfaceReq_t *req = payload;
    bharat_display_broker_v2_PresentSurfaceResp_t resp = {0};

    if (hdr->payload_size < sizeof(*req)) {
        send_reply(hdr->reply_endpoint, hdr, BHARAT_IPC_STATUS_ERR_LENGTH, NULL, 0);
        return;
    }

    bh_direct_scanout_reason_t reason;
    bh_gui_fence_handle_t release_fence;
    resp.result = bh_compositor_present(&g_core, req->lease_handle, req->surface_handle, req->buffer_handle, req->acquire_fence_handle, req->deadline_ns, &reason, &release_fence);
    resp.direct_scanout_reason = reason;
    if (resp.result == BH_DISPLAY_RESULT_OK) {
        resp.release_fence_handle = release_fence;
    }

    send_reply(hdr->reply_endpoint, hdr, BHARAT_STATUS_OK, &resp, sizeof(resp));
}

static void handle_retire_presentation_v2(const bharat_ipc_msg_header_t *hdr, const void *payload) {
    const bharat_display_broker_v2_RetirePresentationReq_t *req = payload;
    bharat_display_broker_v2_RetirePresentationResp_t resp = {0};

    if (hdr->payload_size < sizeof(*req)) {
        send_reply(hdr->reply_endpoint, hdr, BHARAT_IPC_STATUS_ERR_LENGTH, NULL, 0);
        return;
    }

    resp.result = bh_compositor_retire_presentation(&g_core, req->lease_handle, req->surface_handle);
    send_reply(hdr->reply_endpoint, hdr, BHARAT_STATUS_OK, &resp, sizeof(resp));
}

static void handle_query_presentation_status_v2(const bharat_ipc_msg_header_t *hdr, const void *payload) {
    const bharat_display_broker_v2_QueryPresentationStatusReq_t *req = payload;
    bharat_display_broker_v2_QueryPresentationStatusResp_t resp = {0};

    if (hdr->payload_size < sizeof(*req)) {
        send_reply(hdr->reply_endpoint, hdr, BHARAT_IPC_STATUS_ERR_LENGTH, NULL, 0);
        return;
    }

    resp.result = bh_compositor_query_presentation_status(&g_core, req->lease_handle, req->surface_handle, &resp.presentation_state, &resp.active_buffer_handle, &resp.frame_counter);
    send_reply(hdr->reply_endpoint, hdr, BHARAT_STATUS_OK, &resp, sizeof(resp));
}

/* -------------------------------------------------------------
 * Main Dispatch Routing Function
 * ------------------------------------------------------------- */

bharat_status_t bh_service_handle_msg(bh_service_ctx_t *ctx, const bh_msg_t *msg) {
    (void)ctx;

    /* Detect interface version to support legacy V1 alongside newly implemented V2 */
    bool is_v2 = (msg->header.interface_version == 2) || (msg->header.service_id == 23);

    if (is_v2) {
        switch (msg->header.opcode) {
            case BH_DISPLAY_BROKER_V2_OP_ENUMERATE_DISPLAYS:
                handle_enumerate_displays_v2(&msg->header, msg->payload);
                break;
            case BH_DISPLAY_BROKER_V2_OP_QUERY_DISPLAY_MODE:
                handle_query_display_mode_v2(&msg->header, msg->payload);
                break;
            case BH_DISPLAY_BROKER_V2_OP_QUERY_PLANE_CAPABILITIES:
                handle_query_plane_capabilities_v2(&msg->header, msg->payload);
                break;
            case BH_DISPLAY_BROKER_V2_OP_REQUEST_DISPLAY_LEASE:
                handle_request_display_lease_v2(&msg->header, msg->payload);
                break;
            case BH_DISPLAY_BROKER_V2_OP_ACKNOWLEDGE_LEASE_REVOCATION:
                handle_acknowledge_lease_revocation_v2(&msg->header, msg->payload);
                break;
            case BH_DISPLAY_BROKER_V2_OP_RELEASE_DISPLAY_LEASE:
                handle_release_display_lease_v2(&msg->header, msg->payload);
                break;
            case BH_DISPLAY_BROKER_V2_OP_CREATE_SURFACE:
                handle_create_surface_v2(&msg->header, msg->payload);
                break;
            case BH_DISPLAY_BROKER_V2_OP_DESTROY_SURFACE:
                handle_destroy_surface_v2(&msg->header, msg->payload);
                break;
            case BH_DISPLAY_BROKER_V2_OP_REGISTER_BUFFER:
                handle_register_buffer_v2(&msg->header, msg->payload);
                break;
            case BH_DISPLAY_BROKER_V2_OP_RELEASE_BUFFER:
                handle_release_buffer_v2(&msg->header, msg->payload);
                break;
            case BH_DISPLAY_BROKER_V2_OP_ATTACH_BUFFER:
                handle_attach_buffer_v2(&msg->header, msg->payload);
                break;
            case BH_DISPLAY_BROKER_V2_OP_PRESENT_SURFACE:
                handle_present_surface_v2(&msg->header, msg->payload);
                break;
            case BH_DISPLAY_BROKER_V2_OP_RETIRE_PRESENTATION:
                handle_retire_presentation_v2(&msg->header, msg->payload);
                break;
            case BH_DISPLAY_BROKER_V2_OP_QUERY_PRESENTATION_STATUS:
                handle_query_presentation_status_v2(&msg->header, msg->payload);
                break;
            default:
                send_reply(msg->header.reply_endpoint, &msg->header, BHARAT_IPC_STATUS_ERR_OPCODE, NULL, 0);
                break;
        }
    } else {
        /* Legacy V1 Compatibility Dispatch */
        switch (msg->header.opcode) {
            case 1: // RequestLease
                handle_request_lease_v1(&msg->header, msg->payload);
                break;
            case 2: // ReleaseLease
                handle_release_lease_v1(&msg->header, msg->payload);
                break;
            case 3: // Present
                handle_present_v1(&msg->header, msg->payload);
                break;
            default:
                send_reply(msg->header.reply_endpoint, &msg->header, BHARAT_IPC_STATUS_ERR_OPCODE, NULL, 0);
                break;
        }
    }
    return BHARAT_STATUS_OK;
}

int main(void) {
    broker_init();
    bharat_runtime_log("Display Broker V2 started with capability-safe core");

    bh_service_start_info_t info = {
        .service_id = 0x00010007,
        .service_name = "display_broker",
        .endpoint = BHARAT_CAP_INVALID_HANDLE
    };

    return bh_service_main(&info);
}
