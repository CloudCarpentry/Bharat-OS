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
    uint32_t cap_id;
    uint32_t flags;
} bharat_sys_vmm_map_page_args_t;

typedef struct bharat_sys_vmm_unmap_page_args {
    uint64_t vaddr;
    uint32_t cap_id;
} bharat_sys_vmm_unmap_page_args_t;

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

typedef struct bharat_sys_thread_create_args {
    uint32_t process_cap;
    uint64_t entry_point;
    uint64_t out_tid_ptr;
} bharat_sys_thread_create_args_t;

typedef struct bharat_sys_sched_attr_args {
    uint32_t thread_cap;
    uint32_t value;
} bharat_sys_sched_attr_args_t;

typedef struct bharat_sys_mem_alloc_args {
    uint32_t resource_cap;
    uint64_t size;
    uint32_t mem_class;
    uint32_t flags;
    uint64_t out_addr_ptr;
} bharat_sys_mem_alloc_args_t;

typedef struct bharat_sys_fault_domain_args {
    uint32_t cap_id;
    uint64_t attr_ptr;
    uint64_t out_domain_ptr;
    uint32_t thread_cap;
} bharat_sys_fault_domain_args_t;

typedef struct bharat_sys_intent_args {
    uint32_t thread_cap;
    uint64_t intent_ptr;
} bharat_sys_intent_args_t;

#endif /* BHARAT_UAPI_SYSCALL_ARGS_H */
// Add args structs if needed, but since we are trying to keep it simple, we might just pass args directly if they fit in registers.
