#ifndef BHARAT_USER_IPC_H
#define BHARAT_USER_IPC_H

#include <stdint.h>

#include "unistd.h"

enum {
    BHARAT_SYS_ENDPOINT_CREATE = 7,
    BHARAT_SYS_ENDPOINT_SEND = 8,
    BHARAT_SYS_ENDPOINT_RECEIVE = 9,
};

static inline long ipc_endpoint_create(uint32_t* out_send_cap, uint32_t* out_recv_cap) {
    return syscall(BHARAT_SYS_ENDPOINT_CREATE, out_send_cap, out_recv_cap);
}

static inline long ipc_endpoint_send(uint32_t send_cap, const void* payload, uint32_t len) {
    return syscall(BHARAT_SYS_ENDPOINT_SEND, send_cap, payload, len);
}

static inline long ipc_endpoint_receive(uint32_t recv_cap, void* out_payload, uint32_t out_cap, uint32_t* out_len) {
    return syscall(BHARAT_SYS_ENDPOINT_RECEIVE, recv_cap, out_payload, out_cap, out_len);
}

#endif // BHARAT_USER_IPC_H
