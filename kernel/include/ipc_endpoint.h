#ifndef BHARAT_IPC_ENDPOINT_H
#define BHARAT_IPC_ENDPOINT_H

#include <stdint.h>

#include "capability.h"

typedef struct {
    uint32_t msg_len;
    uint8_t payload[64];
} ipc_message_t;

typedef enum {
    IPC_OK = 0,
    IPC_ERR_INVALID = -1,
    IPC_ERR_NO_SPACE = -2,
    IPC_ERR_WOULD_BLOCK = -3,
    IPC_ERR_PERM = -4,
} ipc_status_t;

int ipc_endpoint_create(capability_table_t* table, uint32_t* out_send_cap, uint32_t* out_recv_cap);
int ipc_endpoint_send(capability_table_t* table, uint32_t send_cap, const void* payload, uint32_t payload_len);
int ipc_endpoint_receive(capability_table_t* table, uint32_t recv_cap, void* out_payload, uint32_t out_payload_capacity, uint32_t* out_received_len);

#endif // BHARAT_IPC_ENDPOINT_H
