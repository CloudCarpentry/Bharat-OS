#pragma once
#include <stdint.h>

#include "bharat/msg/wire_types.h"

typedef struct {
    bharat_cap_wire_t cap;
    uint32_t target_node;
} bharat_cap_v1_RemoteGrantReq_t;

typedef struct {
    uint32_t status;
} bharat_cap_v1_RemoteGrantResp_t;

typedef struct {
    uint64_t cap_id;
} bharat_cap_v1_RemoteRevokeReq_t;

typedef struct {
    uint32_t status;
} bharat_cap_v1_RemoteRevokeResp_t;
