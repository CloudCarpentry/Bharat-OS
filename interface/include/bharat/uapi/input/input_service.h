/* SPDX-License-Identifier: MIT */
#ifndef BHARAT_UAPI_INPUT_SERVICE_H
#define BHARAT_UAPI_INPUT_SERVICE_H

#include <stddef.h>
#include <stdint.h>

#include <bharat/uapi/input/input_event.h>
#include <bharat/uapi/services/service_ids.h>

#define BH_INPUT_SERVICE_NAME "inputmgr"
#define BH_INPUT_SERVICE_INTERFACE_VERSION UINT32_C(1)
#define BH_INPUT_SERVICE_OP_DRAIN UINT32_C(1)
#define BH_INPUT_SERVICE_MAX_EVENTS UINT32_C(16)

typedef struct {
  uint32_t max_events;
  uint32_t reserved;
} bh_input_service_drain_request_t;

typedef struct {
  uint32_t event_count;
  uint32_t reserved;
  bh_input_event_t events[BH_INPUT_SERVICE_MAX_EVENTS];
} bh_input_service_drain_response_t;

_Static_assert(sizeof(bh_input_service_drain_request_t) == 8,
               "input drain request ABI size");
_Static_assert(offsetof(bh_input_service_drain_response_t, events) == 8,
               "input drain response event offset");
_Static_assert(sizeof(bh_input_service_drain_response_t) == 392,
               "input drain response ABI size");

#endif /* BHARAT_UAPI_INPUT_SERVICE_H */
