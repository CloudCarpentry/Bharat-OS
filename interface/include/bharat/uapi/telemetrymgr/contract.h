#ifndef BHARAT_UAPI_TELEMETRYMGR_CONTRACT_H
#define BHARAT_UAPI_TELEMETRYMGR_CONTRACT_H

#include <stdint.h>
#include <bharat/system/telemetry.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BHARAT_SERVICE_TELEMETRYMGR 0x00010008

enum {
    TELEMETRYMGR_OP_REGISTER_METRIC = 1,
    TELEMETRYMGR_OP_UPDATE_COUNTER = 2,
    TELEMETRYMGR_OP_UPDATE_GAUGE = 3,
    TELEMETRYMGR_OP_QUERY_METRIC = 4,
    TELEMETRYMGR_OP_REQUEST_SNAPSHOT = 5,
    TELEMETRYMGR_OP_RESET_METRIC = 6,
    TELEMETRYMGR_OP_SHUTDOWN = 7
};

typedef struct {
    bharat_counter_desc_t desc;
} telemetrymgr_req_register_metric_t;

typedef struct {
    uint32_t counter_id;
    int32_t status;
} telemetrymgr_resp_register_metric_t;

typedef struct {
    uint32_t counter_id;
    uint64_t value;
    uint64_t sequence; // Sequence for ordering
} telemetrymgr_req_update_metric_t;

typedef struct {
    int32_t status;
} telemetrymgr_resp_update_metric_t;

typedef struct {
    uint32_t counter_id;
} telemetrymgr_req_query_metric_t;

typedef struct {
    int32_t status;
    bharat_counter_snapshot_t snapshot;
} telemetrymgr_resp_query_metric_t;

typedef struct {
    uint32_t start_index; // For pagination
    uint32_t max_count;
} telemetrymgr_req_request_snapshot_t;

typedef struct {
    int32_t status;
    uint32_t count;
    uint32_t total_count;
    uint32_t overflow_count;
    uint32_t invalid_message_count;
    // Followed by `count` number of bharat_counter_snapshot_t
} telemetrymgr_resp_request_snapshot_t;

typedef struct {
    uint32_t counter_id;
} telemetrymgr_req_reset_metric_t;

typedef struct {
    int32_t status;
} telemetrymgr_resp_reset_metric_t;

#ifdef __cplusplus
}
#endif

#endif // BHARAT_UAPI_TELEMETRYMGR_CONTRACT_H
