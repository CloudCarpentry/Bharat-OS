#pragma once
#include <stdint.h>

typedef struct {
    uint32_t node_id;
    uint64_t monotonic_time_ns;
    uint32_t health_flags;
} bharat_monitor_v1_HeartbeatReq_t;

typedef struct {
    uint32_t accepted;
} bharat_monitor_v1_HeartbeatResp_t;

typedef struct {
    uint32_t protocol_version;
    uint32_t arch;
    uint64_t boot_nonce;
    struct { uint32_t len; char data[64]; } node_name;
} bharat_monitor_v1_NodeJoinReq_t;

typedef struct {
    uint32_t accepted;
    uint32_t assigned_node_id;
    uint64_t session_nonce;
} bharat_monitor_v1_NodeJoinResp_t;

typedef struct {
    uint64_t aspace_id;
    uint64_t va_start;
    uint64_t length;
    uint32_t type;
    uint32_t generation;
} bharat_monitor_v1_TlbInvalidateReq_t;

typedef struct {
    uint32_t status;
} bharat_monitor_v1_TlbInvalidateResp_t;
