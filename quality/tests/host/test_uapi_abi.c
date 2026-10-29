#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include <bharat/uapi/abi_types.h>
#include <bharat/uapi/syscall_nr.h>
#include <bharat/uapi/syscall_args.h>
#include <bharat/uapi/sys_errno.h>
#include <bharat/uapi/service_status.h>
#include <bharat/memory/hmem.h>
#include <bharat/uapi/ipc/envelope.h>

#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
#define STATIC_ASSERT _Static_assert
#else
#define STATIC_ASSERT(cond, msg)
#endif

STATIC_ASSERT(BHARAT_SYSCALL_ABI_VERSION_MAJOR == 1U, "ABI major changed");
STATIC_ASSERT(BHARAT_SYSCALL_ABI_VERSION_MINOR == 0U, "ABI minor changed");

STATIC_ASSERT(SYSCALL_NOP == 0, "syscall number drift");
STATIC_ASSERT(SYSCALL_THREAD_CREATE == 1, "syscall number drift");
STATIC_ASSERT(SYSCALL_THREAD_DESTROY == 2, "syscall number drift");
STATIC_ASSERT(SYSCALL_SCHED_YIELD == 3, "syscall number drift");
STATIC_ASSERT(SYSCALL_VMM_MAP_PAGE == 4, "syscall number drift");
STATIC_ASSERT(SYSCALL_VMM_UNMAP_PAGE == 5, "syscall number drift");
STATIC_ASSERT(SYSCALL_CAPABILITY_INVOKE == 6, "syscall number drift");
STATIC_ASSERT(SYSCALL_ENDPOINT_CREATE == 7, "syscall number drift");
STATIC_ASSERT(SYSCALL_ENDPOINT_SEND == 8, "syscall number drift");
STATIC_ASSERT(SYSCALL_ENDPOINT_RECEIVE == 9, "syscall number drift");
STATIC_ASSERT(SYSCALL_CAPABILITY_DELEGATE == 10, "syscall number drift");
STATIC_ASSERT(SYSCALL_SCHED_SLEEP == 11, "syscall number drift");
STATIC_ASSERT(SYSCALL_SCHED_SET_PRIORITY == 12, "syscall number drift");
STATIC_ASSERT(SYSCALL_SCHED_SET_AFFINITY == 13, "syscall number drift");

STATIC_ASSERT(sizeof(bharat_handle_t) == 8, "handle width drift");
STATIC_ASSERT(sizeof(bharat_object_id_t) == 8, "object id width drift");
STATIC_ASSERT(sizeof(bharat_cap_id_t) == 8, "cap id width drift");

STATIC_ASSERT(sizeof(bharat_sys_endpoint_create_args_t) == 16, "endpoint_create abi drift");
STATIC_ASSERT(sizeof(bharat_sys_endpoint_send_args_t) == 40, "endpoint_send abi drift");
STATIC_ASSERT(sizeof(bharat_sys_endpoint_receive_args_t) == 40, "endpoint_receive abi drift");
STATIC_ASSERT(sizeof(bharat_sys_cap_invoke_args_t) == 32, "cap_invoke abi drift");
STATIC_ASSERT(sizeof(bharat_sys_vmm_map_page_args_t) == 16, "vmm_map abi drift");
STATIC_ASSERT(sizeof(bharat_sys_cap_delegate_args_t) == 24, "cap_delegate abi drift");

STATIC_ASSERT(sizeof(bh_hmem_create_args_t) == 112, "hmem_create abi drift");
STATIC_ASSERT(sizeof(bh_hmem_query_args_t) == 96, "hmem_query abi drift");
STATIC_ASSERT(sizeof(bh_hmem_map_cpu_args_t) == 32, "hmem_map_cpu abi drift");
STATIC_ASSERT(sizeof(bh_hmem_unmap_cpu_args_t) == 16, "hmem_unmap_cpu abi drift");
STATIC_ASSERT(sizeof(bh_hmem_sync_args_t) == 32, "hmem_sync abi drift");
STATIC_ASSERT(offsetof(bh_hmem_create_args_t, resource_cap) == 104, "hmem_create resource_cap offset drift");

STATIC_ASSERT(offsetof(bharat_sys_endpoint_send_args_t, payload_ptr) == 8, "endpoint_send layout drift");
STATIC_ASSERT(offsetof(bharat_sys_endpoint_receive_args_t, out_len_ptr) == 16, "endpoint_receive layout drift");
STATIC_ASSERT(offsetof(bharat_sys_cap_delegate_args_t, out_cap_ptr) == 16, "cap_delegate layout drift");

STATIC_ASSERT(sizeof(bharat_sys_endpoint_receive_v1_args_t) == 64, "endpoint_receive_v1 abi drift");
STATIC_ASSERT(offsetof(bharat_sys_endpoint_receive_v1_args_t, version) == 0, "endpoint_receive_v1 layout drift");
STATIC_ASSERT(offsetof(bharat_sys_endpoint_receive_v1_args_t, struct_size) == 4, "endpoint_receive_v1 layout drift");
STATIC_ASSERT(offsetof(bharat_sys_endpoint_receive_v1_args_t, flags) == 8, "endpoint_receive_v1 layout drift");
STATIC_ASSERT(offsetof(bharat_sys_endpoint_receive_v1_args_t, recv_cap) == 12, "endpoint_receive_v1 layout drift");
STATIC_ASSERT(offsetof(bharat_sys_endpoint_receive_v1_args_t, out_payload_capacity) == 16, "endpoint_receive_v1 layout drift");
STATIC_ASSERT(offsetof(bharat_sys_endpoint_receive_v1_args_t, out_envelope_capacity) == 20, "endpoint_receive_v1 layout drift");
STATIC_ASSERT(offsetof(bharat_sys_endpoint_receive_v1_args_t, out_payload_ptr) == 24, "endpoint_receive_v1 layout drift");
STATIC_ASSERT(offsetof(bharat_sys_endpoint_receive_v1_args_t, out_len_ptr) == 32, "endpoint_receive_v1 layout drift");
STATIC_ASSERT(offsetof(bharat_sys_endpoint_receive_v1_args_t, timeout_ticks) == 40, "endpoint_receive_v1 layout drift");
STATIC_ASSERT(offsetof(bharat_sys_endpoint_receive_v1_args_t, out_envelope_ptr) == 48, "endpoint_receive_v1 layout drift");
STATIC_ASSERT(offsetof(bharat_sys_endpoint_receive_v1_args_t, reserved0) == 56, "endpoint_receive_v1 layout drift");

STATIC_ASSERT(sizeof(bh_ipc_receive_envelope_v1_t) == 96, "receive envelope v1 abi drift");
STATIC_ASSERT(offsetof(bh_ipc_receive_envelope_v1_t, version) == 0, "receive envelope layout drift");
STATIC_ASSERT(offsetof(bh_ipc_receive_envelope_v1_t, struct_size) == 4, "receive envelope layout drift");
STATIC_ASSERT(offsetof(bh_ipc_receive_envelope_v1_t, valid_fields) == 8, "receive envelope layout drift");
STATIC_ASSERT(offsetof(bh_ipc_receive_envelope_v1_t, sender_pid) == 16, "receive envelope layout drift");
STATIC_ASSERT(offsetof(bh_ipc_receive_envelope_v1_t, sender_process_object_id) == 24, "receive envelope layout drift");
STATIC_ASSERT(offsetof(bh_ipc_receive_envelope_v1_t, boot_session_id) == 32, "receive envelope layout drift");
STATIC_ASSERT(offsetof(bh_ipc_receive_envelope_v1_t, service_incarnation_id) == 40, "receive envelope layout drift");
STATIC_ASSERT(offsetof(bh_ipc_receive_envelope_v1_t, sender_cspace_id) == 48, "receive envelope layout drift");
STATIC_ASSERT(offsetof(bh_ipc_receive_envelope_v1_t, sender_owner_core) == 52, "receive envelope layout drift");
STATIC_ASSERT(offsetof(bh_ipc_receive_envelope_v1_t, authority_cap) == 56, "receive envelope layout drift");
STATIC_ASSERT(offsetof(bh_ipc_receive_envelope_v1_t, reply_send_cap) == 60, "receive envelope layout drift");
STATIC_ASSERT(offsetof(bh_ipc_receive_envelope_v1_t, authority_object_type) == 64, "receive envelope layout drift");
STATIC_ASSERT(offsetof(bh_ipc_receive_envelope_v1_t, reserved0) == 68, "receive envelope layout drift");
STATIC_ASSERT(offsetof(bh_ipc_receive_envelope_v1_t, authority_rights) == 72, "receive envelope layout drift");
STATIC_ASSERT(offsetof(bh_ipc_receive_envelope_v1_t, message_id) == 80, "receive envelope layout drift");
STATIC_ASSERT(offsetof(bh_ipc_receive_envelope_v1_t, delivery_generation) == 88, "receive envelope layout drift");


STATIC_ASSERT(SYS_EINVAL == 22, "sys errno drift");
STATIC_ASSERT(SYS_ENOSYS == 38, "sys errno drift");

STATIC_ASSERT(BHARAT_STATUS_OK == 0, "service status drift");
STATIC_ASSERT(BHARAT_STATUS_ERR_VERSION == -2, "service status drift");
STATIC_ASSERT(BHARAT_STATUS_ERR_PERMISSION == -4, "service status drift");

int main(void) {
    assert(BHARAT_INVALID_HANDLE == 0ULL);
    assert(BHARAT_INVALID_OBJECT_ID == 0ULL);
    assert(BHARAT_INVALID_CAP_ID == 0ULL);

    printf("UAPI ABI drift guards passed.\n");
    return 0;
}
