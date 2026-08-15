#ifndef BHARAT_UAPI_CONSOLE_LOG_H
#define BHARAT_UAPI_CONSOLE_LOG_H

#include <stdint.h>
#include <bharat/uapi/service_status.h>

#ifdef __cplusplus
extern "C" {
#endif

// Opcodes for the Console API
#define BH_CONSOLE_OP_OPEN_LOG_STREAM 10
#define BH_CONSOLE_OP_READ_RECORDS    11
#define BH_CONSOLE_OP_SUBSCRIBE       12
#define BH_CONSOLE_OP_SET_MIN_LEVEL   13

typedef struct {
    uint64_t sequence;
    uint64_t timestamp_ns;
    uint16_t source;
    uint8_t level;
    uint8_t flags;
    char text[192];
} bh_console_record_v1_t;

typedef struct {
    uint8_t min_level;
} bh_console_subscribe_req_t;

typedef struct {
    uint32_t handle;
    bharat_status_t result;
} bh_console_subscribe_resp_t;

typedef struct {
    uint32_t handle;
    uint32_t max_records;
} bh_console_read_req_t;

typedef struct {
    bharat_status_t result;
    uint32_t count;
    // Followed by `count` records of `bh_console_record_v1_t`
} bh_console_read_resp_t;

#ifdef __cplusplus
}
#endif

#endif /* BHARAT_UAPI_CONSOLE_LOG_H */
