#ifndef BHARAT_UAPI_SYSCALL_ARGS_H
#define BHARAT_UAPI_SYSCALL_ARGS_H

#include <stdint.h>
#include <bharat/uapi/abi_types.h>

/*
 * Strict syscall argument carriers for complex syscalls.
 * Userspace passes a pointer to one of these structs in arg0.
 */

typedef struct bharat_sys_vmm_map_page_args {
    uint64_t vaddr;
    uint64_t paddr;
    uint32_t flags;
    uint32_t reserved0;
} bharat_sys_vmm_map_page_args_t;

typedef struct bharat_sys_cap_invoke_args {
    uint64_t cap_id;
    uint64_t opcode;
    uint64_t arg0;
    uint64_t arg1;
} bharat_sys_cap_invoke_args_t;

typedef struct bharat_sys_endpoint_create_args {
    uint64_t out_send_cap_ptr;
    uint64_t out_recv_cap_ptr;
} bharat_sys_endpoint_create_args_t;

typedef struct bharat_sys_endpoint_send_args {
    uint32_t send_cap;
    uint32_t payload_len;
    uint64_t payload_ptr;
    uint64_t timeout_ticks;
    uint32_t cap_to_send;
    uint32_t cap_send_rights;
    uint64_t reserved0;
} bharat_sys_endpoint_send_args_t;

typedef struct bharat_sys_endpoint_receive_args {
    uint32_t recv_cap;
    uint32_t out_payload_capacity;
    uint64_t out_payload_ptr;
    uint64_t out_len_ptr;
    uint64_t timeout_ticks;
    uint64_t out_received_cap_ptr;
} bharat_sys_endpoint_receive_args_t;

typedef struct bharat_sys_cap_delegate_args {
    uint32_t src_cap;
    uint32_t requested_rights;
    uint64_t out_cap_ptr;
} bharat_sys_cap_delegate_args_t;

#endif /* BHARAT_UAPI_SYSCALL_ARGS_H */
