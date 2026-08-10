#ifndef BH_ACCELMGR_TEST_FAKE_IPC_H
#define BH_ACCELMGR_TEST_FAKE_IPC_H

#include <bharat/ipc/ipc.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint32_t call_count;
    bharat_ipc_endpoint_t last_endpoint;
    bharat_ipc_msg_header_t last_header;
    uint8_t last_payload[128];
    uint32_t last_payload_size;
    int32_t configured_result;
} bh_test_ipc_state_t;

void bh_test_ipc_reset(void);
bh_test_ipc_state_t *bh_test_ipc_state(void);

#endif
