/* SPDX-License-Identifier: MIT */
#include <stddef.h>
#include <string.h>

#include <bharat/ipc/ipc.h>
#include <bharat/namesvc/client.h>
#include <bharat/uapi/input/input_service.h>

static bharat_ipc_endpoint_t g_input_endpoint = BHARAT_CAP_INVALID_HANDLE;

static int connect_input_service(void) {
  bharat_service_id_t service_id = 0;
  uint32_t interface_version = 0;

  if (g_input_endpoint != BHARAT_CAP_INVALID_HANDLE) {
    return 0;
  }
  if (namesvc_lookup(BH_INPUT_SERVICE_NAME, &service_id, &g_input_endpoint,
                     &interface_version) != 0 ||
      service_id != BHARAT_SERVICE_INPUTMGR_V1 ||
      interface_version != BH_INPUT_SERVICE_INTERFACE_VERSION ||
      g_input_endpoint == BHARAT_CAP_INVALID_HANDLE) {
    g_input_endpoint = BHARAT_CAP_INVALID_HANDLE;
    return -1;
  }
  return 0;
}

int bh_inputmgr_drain(bh_input_event_t *out_events, int max_events) {
  bh_input_service_drain_request_t request = {0};
  bh_input_service_drain_response_t response = {0};
  bharat_ipc_msg_header_t request_header = {
      .header_version = BHARAT_IPC_HEADER_VERSION_V1,
      .service_id = BHARAT_SERVICE_INPUTMGR_V1,
      .interface_version = BH_INPUT_SERVICE_INTERFACE_VERSION,
      .opcode = BH_INPUT_SERVICE_OP_DRAIN,
      .payload_size = sizeof(request),
  };
  bharat_ipc_msg_header_t response_header = {0};
  uint32_t limit;

  if (out_events == NULL || max_events <= 0 || connect_input_service() != 0) {
    return 0;
  }
  limit = (uint32_t)max_events;
  if (limit > BH_INPUT_SERVICE_MAX_EVENTS) {
    limit = BH_INPUT_SERVICE_MAX_EVENTS;
  }
  request.max_events = limit;
  if (bharat_ipc_call(g_input_endpoint, &request_header, &request,
                      &response_header, &response, sizeof(response)) != 0 ||
      response_header.status != BHARAT_STATUS_OK ||
      response_header.service_id != BHARAT_SERVICE_INPUTMGR_V1 ||
      response_header.interface_version != BH_INPUT_SERVICE_INTERFACE_VERSION ||
      response_header.opcode != BH_INPUT_SERVICE_OP_DRAIN ||
      response_header.payload_size != sizeof(response) ||
      response.event_count > limit) {
    g_input_endpoint = BHARAT_CAP_INVALID_HANDLE;
    return 0;
  }
  memcpy(out_events, response.events,
         (size_t)response.event_count * sizeof(response.events[0]));
  return (int)response.event_count;
}
