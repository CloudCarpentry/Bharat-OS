long bh_syscall(long sysno, long arg1, long arg2, long arg3, long arg4, long arg5, long arg6);
#include <bharat/syscalls.h>
#ifndef BHARAT_USER_IPC_H
#define BHARAT_USER_IPC_H

#include <stdint.h>

#include <bharat/uapi/syscall_nr.h>
#include <bharat/uapi/syscall_args.h>
#include <bharat/uapi/ipc/contract.h>

long bh_syscall(long sysno, long arg1, long arg2, long arg3, long arg4, long arg5, long arg6);

static inline long ipc_endpoint_create(uint32_t* out_send_cap, uint32_t* out_recv_cap) {
    bharat_sys_endpoint_create_args_t args = {
        .out_send_cap_ptr = (uint64_t)(uintptr_t)out_send_cap,
        .out_recv_cap_ptr = (uint64_t)(uintptr_t)out_recv_cap,
    };
    return bh_syscall(SYSCALL_ENDPOINT_CREATE, (long)(uintptr_t)&args, 0, 0, 0, 0, 0);
}

static inline long ipc_endpoint_close(uint32_t cap) {
    if (!cap) return 0;
    bharat_sys_cap_invoke_args_t args = {
        .cap_id = cap,
        .opcode = BH_ENDPOINT_OP_CLOSE,
        .arg0 = 0,
        .arg1 = 0,
    };
    return bh_syscall(SYSCALL_CAPABILITY_INVOKE, (long)(uintptr_t)&args, 0, 0, 0, 0, 0);
}

static inline long ipc_endpoint_send_ex(uint32_t send_cap,
                                        const void* payload,
                                        uint32_t len,
                                        uint64_t timeout_ticks,
                                        uint32_t cap_to_send,
                                        uint64_t cap_rights,
                                        uint32_t reply_cap_to_send,
                                        uint64_t reply_cap_rights) {
    bharat_sys_endpoint_send_args_t args = {
        .send_cap = send_cap,
        .payload_len = len,
        .payload_ptr = (uint64_t)(uintptr_t)payload,
        .timeout_ticks = timeout_ticks,
        .cap_to_send = cap_to_send,
        .cap_send_rights = (uint32_t)cap_rights,
        .reply_cap_to_send = reply_cap_to_send,
        .reply_cap_send_rights = (uint32_t)reply_cap_rights,
    };
    return bh_syscall(SYSCALL_ENDPOINT_SEND, (long)(uintptr_t)&args, 0, 0, 0, 0, 0);
}

static inline long ipc_endpoint_send(uint32_t send_cap,
                                     const void* payload,
                                     uint32_t len,
                                     uint64_t timeout_ticks,
                                     uint32_t cap_to_send,
                                     uint32_t cap_rights) {
    return ipc_endpoint_send_ex(send_cap, payload, len, timeout_ticks, cap_to_send, cap_rights, 0U, 0U);
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
    return bh_syscall(SYSCALL_ENDPOINT_RECEIVE, (long)(uintptr_t)&args, (long)BH_IPC_RECEIVE_SELECTOR_LEGACY, 0, 0, 0, 0);
}

static inline long ipc_endpoint_receive_envelope(uint32_t recv_cap,
                                                 void* out_payload,
                                                 uint32_t out_capacity,
                                                 uint32_t* out_len,
                                                 uint64_t timeout_ticks,
                                                 bh_ipc_receive_envelope_v1_t* out_envelope) {
    bharat_sys_endpoint_receive_v1_args_t args = {
        .version = BH_IPC_RECEIVE_ENVELOPE_VERSION_V1,
        .struct_size = (uint32_t)sizeof(bharat_sys_endpoint_receive_v1_args_t),
        .flags = 0,
        .recv_cap = recv_cap,
        .out_payload_capacity = out_capacity,
        .out_envelope_capacity = (uint32_t)sizeof(bh_ipc_receive_envelope_v1_t),
        .out_payload_ptr = (uint64_t)(uintptr_t)out_payload,
        .out_len_ptr = (uint64_t)(uintptr_t)out_len,
        .out_envelope_ptr = (uint64_t)(uintptr_t)out_envelope,
        .timeout_ticks = timeout_ticks,
        .reserved0 = 0,
    };
    return bh_syscall(SYSCALL_ENDPOINT_RECEIVE, (long)(uintptr_t)&args, (long)BH_IPC_RECEIVE_SELECTOR_V1, 0, 0, 0, 0);
}

#endif // BHARAT_USER_IPC_H
