#ifndef BHARAT_USER_IPC_H
#define BHARAT_USER_IPC_H

#include <stdint.h>

#include <bharat/uapi/syscall_nr.h>
#include <bharat/uapi/syscall_args.h>

long bharat_syscall(long sysno, long arg1, long arg2, long arg3, long arg4, long arg5, long arg6);

static inline long ipc_endpoint_create(uint32_t* out_send_cap, uint32_t* out_recv_cap) {
    bharat_sys_endpoint_create_args_t args = {
        .out_send_cap_ptr = (uint64_t)(uintptr_t)out_send_cap,
        .out_recv_cap_ptr = (uint64_t)(uintptr_t)out_recv_cap,
    };
    return bharat_syscall(SYSCALL_ENDPOINT_CREATE, (long)(uintptr_t)&args, 0, 0, 0, 0, 0);
}

static inline long ipc_endpoint_send(uint32_t send_cap,
                                     const void* payload,
                                     uint32_t len,
                                     uint64_t timeout_ticks,
                                     uint32_t cap_to_send,
                                     uint32_t cap_rights) {
    bharat_sys_endpoint_send_args_t args = {
        .send_cap = send_cap,
        .payload_len = len,
        .payload_ptr = (uint64_t)(uintptr_t)payload,
        .timeout_ticks = timeout_ticks,
        .cap_to_send = cap_to_send,
        .cap_send_rights = cap_rights,
        .reserved0 = 0,
    };
    return bharat_syscall(SYSCALL_ENDPOINT_SEND, (long)(uintptr_t)&args, 0, 0, 0, 0, 0);
}

static inline long ipc_endpoint_receive(uint32_t recv_cap,
                                        void* out_payload,
                                        uint32_t out_capacity,
                                        uint32_t* out_len,
                                        uint64_t timeout_ticks,
                                        uint32_t* out_received_cap) {
    bharat_sys_endpoint_receive_args_t args = {
        .recv_cap = recv_cap,
        .out_payload_capacity = out_capacity,
        .out_payload_ptr = (uint64_t)(uintptr_t)out_payload,
        .out_len_ptr = (uint64_t)(uintptr_t)out_len,
        .timeout_ticks = timeout_ticks,
        .out_received_cap_ptr = (uint64_t)(uintptr_t)out_received_cap,
    };
    return bharat_syscall(SYSCALL_ENDPOINT_RECEIVE, (long)(uintptr_t)&args, 0, 0, 0, 0, 0);
}

#endif // BHARAT_USER_IPC_H
