/* SPDX-License-Identifier: MIT */
#include "display_client.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "bharat/uapi/display/bharat_display_broker_v2_types.h"
#include "bharat/uapi/display/lease.h"
#include <bharat/ipc/ipc.h>
#include <bharat/namesvc/client.h>

#define BH_SHOWCASE_DISPLAY_SERVICE_NAME "display_broker"
#define BH_SHOWCASE_DISPLAY_SERVICE_ID UINT32_C(23)
#define BH_SHOWCASE_DISPLAY_INTERFACE_VERSION UINT32_C(2)

static bharat_ipc_endpoint_t g_display_endpoint = BHARAT_CAP_INVALID_HANDLE;

static bh_display_result_t display_call(uint32_t opcode, const void *request,
                                        uint32_t request_size, void *response,
                                        uint32_t response_size) {
  bharat_ipc_msg_header_t request_header = {
      .header_version = BHARAT_IPC_HEADER_VERSION_V1,
      .service_id = BH_SHOWCASE_DISPLAY_SERVICE_ID,
      .interface_version = BH_SHOWCASE_DISPLAY_INTERFACE_VERSION,
      .opcode = opcode,
      .payload_size = request_size,
  };
  bharat_ipc_msg_header_t response_header = {0};

  if (g_display_endpoint == BHARAT_CAP_INVALID_HANDLE ||
      bharat_ipc_call(g_display_endpoint, &request_header, request,
                      &response_header, response, response_size) != 0 ||
      response_header.status != BHARAT_STATUS_OK ||
      response_header.payload_size > response_size) {
    return BH_DISPLAY_RESULT_INTERNAL;
  }
  return BH_DISPLAY_RESULT_OK;
}

static bh_display_result_t connect_display_broker(void) {
  bharat_service_id_t service_id = 0;
  uint32_t interface_version = 0;

  if (g_display_endpoint != BHARAT_CAP_INVALID_HANDLE) {
    return BH_DISPLAY_RESULT_OK;
  }
  if (namesvc_lookup(BH_SHOWCASE_DISPLAY_SERVICE_NAME, &service_id,
                     &g_display_endpoint, &interface_version) != 0 ||
      service_id != BH_SHOWCASE_DISPLAY_SERVICE_ID ||
      interface_version < BH_SHOWCASE_DISPLAY_INTERFACE_VERSION ||
      g_display_endpoint == BHARAT_CAP_INVALID_HANDLE) {
    g_display_endpoint = BHARAT_CAP_INVALID_HANDLE;
    return BH_DISPLAY_RESULT_NOT_FOUND;
  }
  return BH_DISPLAY_RESULT_OK;
}

bh_display_result_t
bh_showcase_display_open(bh_showcase_display_session_t *session) {
  bharat_display_broker_v2_EnumerateDisplaysReq_t enumerate_request = {0};
  bharat_display_broker_v2_EnumerateDisplaysResp_t enumerate_response = {0};
  bharat_display_broker_v2_QueryDisplayModeReq_t mode_request = {0};
  bharat_display_broker_v2_QueryDisplayModeResp_t mode_response = {0};
  bharat_display_broker_v2_RequestDisplayLeaseReq_t lease_request = {0};
  bharat_display_broker_v2_RequestDisplayLeaseResp_t lease_response = {0};
  bh_display_result_t result;

  if (session == NULL) {
    return BH_DISPLAY_RESULT_INVALID_ARGUMENT;
  }
  memset(session, 0, sizeof(*session));

  result = connect_display_broker();
  if (result != BH_DISPLAY_RESULT_OK) {
    return result;
  }
  result = display_call(BH_DISPLAY_BROKER_V2_OP_ENUMERATE_DISPLAYS,
                        &enumerate_request, sizeof(enumerate_request),
                        &enumerate_response, sizeof(enumerate_response));
  if (result != BH_DISPLAY_RESULT_OK) {
    return result;
  }
  if (enumerate_response.result != BH_DISPLAY_RESULT_OK ||
      enumerate_response.display_count == 0 ||
      !bh_gui_handle_validate(enumerate_response.display_handle_0,
                              BH_GUI_RESOURCE_DISPLAY)) {
    return enumerate_response.result == BH_DISPLAY_RESULT_OK
               ? BH_DISPLAY_RESULT_NOT_FOUND
               : (bh_display_result_t)enumerate_response.result;
  }

  mode_request.display_handle = enumerate_response.display_handle_0;
  result =
      display_call(BH_DISPLAY_BROKER_V2_OP_QUERY_DISPLAY_MODE, &mode_request,
                   sizeof(mode_request), &mode_response, sizeof(mode_response));
  if (result != BH_DISPLAY_RESULT_OK) {
    return result;
  }
  if (mode_response.result != BH_DISPLAY_RESULT_OK ||
      mode_response.width == 0 || mode_response.height == 0 ||
      mode_response.pixel_format != BH_DISPLAY_FORMAT_XRGB8888) {
    return mode_response.result == BH_DISPLAY_RESULT_OK
               ? BH_DISPLAY_RESULT_FORMAT_UNSUPPORTED
               : (bh_display_result_t)mode_response.result;
  }

  lease_request.display_handle = enumerate_response.display_handle_0;
  lease_request.requested_rights = BHARAT_DISPLAY_RIGHT_LEASE |
                                   BHARAT_DISPLAY_RIGHT_WRITE |
                                   BHARAT_DISPLAY_RIGHT_PRESENT;
  result = display_call(BH_DISPLAY_BROKER_V2_OP_REQUEST_DISPLAY_LEASE,
                        &lease_request, sizeof(lease_request), &lease_response,
                        sizeof(lease_response));
  if (result != BH_DISPLAY_RESULT_OK) {
    return result;
  }
  if (lease_response.result != BH_DISPLAY_RESULT_OK ||
      !bh_gui_handle_validate(lease_response.lease_handle,
                              BH_GUI_RESOURCE_LEASE) ||
      (lease_response.granted_rights & lease_request.requested_rights) !=
          lease_request.requested_rights) {
    return lease_response.result == BH_DISPLAY_RESULT_OK
               ? BH_DISPLAY_RESULT_PERMISSION_DENIED
               : (bh_display_result_t)lease_response.result;
  }

  session->display = enumerate_response.display_handle_0;
  session->lease = lease_response.lease_handle;
  session->width = mode_response.width;
  session->height = mode_response.height;
  session->refresh_hz = mode_response.refresh_hz;
  session->pixel_format = mode_response.pixel_format;
  return BH_DISPLAY_RESULT_OK;
}

bh_display_result_t
bh_client_create_surface(bh_display_lease_handle_t lease, uint32_t width,
                         uint32_t height, uint32_t z_order,
                         bh_gui_surface_handle_t *out_surface) {
  bharat_display_broker_v2_CreateSurfaceReq_t request = {
      .lease_handle = lease,
      .width = width,
      .height = height,
      .z_order = z_order,
  };
  bharat_display_broker_v2_CreateSurfaceResp_t response = {0};
  bh_display_result_t result;

  if (out_surface == NULL) {
    return BH_DISPLAY_RESULT_INVALID_ARGUMENT;
  }
  result = display_call(BH_DISPLAY_BROKER_V2_OP_CREATE_SURFACE, &request,
                        sizeof(request), &response, sizeof(response));
  if (result != BH_DISPLAY_RESULT_OK) {
    return result;
  }
  if (response.result == BH_DISPLAY_RESULT_OK) {
    if (!bh_gui_handle_validate(response.surface_handle,
                                BH_GUI_RESOURCE_SURFACE)) {
      return BH_DISPLAY_RESULT_INTERNAL;
    }
    *out_surface = response.surface_handle;
  }
  return (bh_display_result_t)response.result;
}

bh_display_result_t bh_client_register_buffer(
    bh_display_lease_handle_t lease, bh_display_buffer_desc_t *desc,
    bh_gui_buffer_handle_t *out_buffer, void **out_mapped) {
  bharat_display_broker_v2_RegisterBufferReq_t request = {0};
  bharat_display_broker_v2_RegisterBufferResp_t response = {0};
  bh_display_result_t result;
  void *mapping;

  if (desc == NULL || out_buffer == NULL || out_mapped == NULL ||
      desc->plane_count != 1 || desc->total_size_bytes == 0 ||
      desc->total_size_bytes > (uint64_t)(size_t)-1) {
    return BH_DISPLAY_RESULT_INVALID_ARGUMENT;
  }
  mapping = malloc((size_t)desc->total_size_bytes);
  if (mapping == NULL) {
    return BH_DISPLAY_RESULT_NO_RESOURCES;
  }
  memset(mapping, 0, (size_t)desc->total_size_bytes);

  request.lease_handle = lease;
  request.width = desc->width;
  request.height = desc->height;
  request.pixel_format = desc->pixel_format;
  request.usage_flags = desc->usage_flags;
  request.memory_domain = desc->memory_domain;
  request.plane_count = desc->plane_count;
  request.total_size_bytes = desc->total_size_bytes;
  request.modifier = desc->modifier;
  request.plane0_offset_bytes = desc->planes[0].offset_bytes;
  request.plane0_size_bytes = desc->planes[0].size_bytes;
  request.plane0_stride_bytes = desc->planes[0].stride_bytes;

  result = display_call(BH_DISPLAY_BROKER_V2_OP_REGISTER_BUFFER, &request,
                        sizeof(request), &response, sizeof(response));
  if (result != BH_DISPLAY_RESULT_OK ||
      response.result != BH_DISPLAY_RESULT_OK ||
      !bh_gui_handle_validate(response.buffer_handle, BH_GUI_RESOURCE_BUFFER)) {
    free(mapping);
    return result != BH_DISPLAY_RESULT_OK
               ? result
               : (response.result == BH_DISPLAY_RESULT_OK
                      ? BH_DISPLAY_RESULT_INTERNAL
                      : (bh_display_result_t)response.result);
  }
  *out_buffer = response.buffer_handle;
  *out_mapped = mapping;
  return BH_DISPLAY_RESULT_OK;
}

bh_display_result_t bh_client_attach_buffer(bh_display_lease_handle_t lease,
                                            bh_gui_surface_handle_t surface,
                                            bh_gui_buffer_handle_t buffer) {
  bharat_display_broker_v2_AttachBufferReq_t request = {
      .lease_handle = lease,
      .surface_handle = surface,
      .buffer_handle = buffer,
  };
  bharat_display_broker_v2_AttachBufferResp_t response = {0};
  bh_display_result_t result =
      display_call(BH_DISPLAY_BROKER_V2_OP_ATTACH_BUFFER, &request,
                   sizeof(request), &response, sizeof(response));
  return result == BH_DISPLAY_RESULT_OK ? (bh_display_result_t)response.result
                                        : result;
}

bh_display_result_t bh_client_present_surface(
    bh_display_lease_handle_t lease, bh_gui_surface_handle_t surface,
    bh_gui_buffer_handle_t buffer, bh_gui_fence_handle_t *out_release_fence) {
  bharat_display_broker_v2_PresentSurfaceReq_t request = {
      .lease_handle = lease,
      .surface_handle = surface,
      .buffer_handle = buffer,
      .acquire_fence_handle = BH_GUI_HANDLE_INVALID,
      .deadline_ns = 0,
  };
  bharat_display_broker_v2_PresentSurfaceResp_t response = {0};
  bh_display_result_t result;

  if (out_release_fence == NULL) {
    return BH_DISPLAY_RESULT_INVALID_ARGUMENT;
  }
  result = display_call(BH_DISPLAY_BROKER_V2_OP_PRESENT_SURFACE, &request,
                        sizeof(request), &response, sizeof(response));
  if (result == BH_DISPLAY_RESULT_OK &&
      response.result == BH_DISPLAY_RESULT_OK) {
    *out_release_fence = response.release_fence_handle;
  }
  return result == BH_DISPLAY_RESULT_OK ? (bh_display_result_t)response.result
                                        : result;
}

bh_display_result_t bh_client_wait_fence(bh_gui_fence_handle_t fence,
                                         bh_monotonic_deadline_ns_t deadline) {
  (void)deadline;
  return fence == BH_GUI_HANDLE_INVALID ? BH_DISPLAY_RESULT_OK
                                        : BH_DISPLAY_RESULT_FEATURE_DISABLED;
}

bh_display_result_t
bh_showcase_display_confirm_presented(bh_display_lease_handle_t lease,
                                      bh_gui_surface_handle_t surface,
                                      bh_gui_buffer_handle_t expected_buffer) {
  bharat_display_broker_v2_QueryPresentationStatusReq_t request = {
      .lease_handle = lease,
      .surface_handle = surface,
  };
  bharat_display_broker_v2_QueryPresentationStatusResp_t response = {0};
  bh_display_result_t result =
      display_call(BH_DISPLAY_BROKER_V2_OP_QUERY_PRESENTATION_STATUS, &request,
                   sizeof(request), &response, sizeof(response));

  if (result != BH_DISPLAY_RESULT_OK) {
    return result;
  }
  if (response.result != BH_DISPLAY_RESULT_OK) {
    return (bh_display_result_t)response.result;
  }
  if (response.frame_counter == 0 ||
      response.active_buffer_handle != expected_buffer) {
    return BH_DISPLAY_RESULT_BAD_STATE;
  }
  return BH_DISPLAY_RESULT_OK;
}
