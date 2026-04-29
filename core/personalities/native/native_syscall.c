#include "trap/syscall_context.h"
#include <bharat/uapi/syscall/bh_syscall_numbers.h>
#include <bharat/uapi/capability/rights.h>
#include "capability.h"

/* Temporary shims for missing UAPI rights */
#ifndef BH_CAP_RIGHT_RESOURCE_ALLOC
#define BH_CAP_RIGHT_RESOURCE_ALLOC BH_CAP_RIGHT_CONTROL
#endif
#ifndef BH_CAP_RIGHT_PROCESS_MANAGE
#define BH_CAP_RIGHT_PROCESS_MANAGE BH_CAP_RIGHT_CONTROL
#endif
#ifndef BH_CAP_RIGHT_MEMORY_MAP
#define BH_CAP_RIGHT_MEMORY_MAP BH_CAP_RIGHT_MAP
#endif
#ifndef BH_CAP_RIGHT_MEMORY_UNMAP
#define BH_CAP_RIGHT_MEMORY_UNMAP BH_CAP_RIGHT_UNMAP
#endif
#ifndef BH_CAP_RIGHT_CRYPT_USE
#define BH_CAP_RIGHT_CRYPT_USE BH_CAP_RIGHT_EXECUTE
#endif
#ifndef BH_CAP_RIGHT_ENDPOINT_SEND
#define BH_CAP_RIGHT_ENDPOINT_SEND BH_CAP_RIGHT_SEND
#endif
#ifndef BH_CAP_RIGHT_ENDPOINT_RECEIVE
#define BH_CAP_RIGHT_ENDPOINT_RECEIVE BH_CAP_RIGHT_RECEIVE
#endif
#ifndef BH_CAP_RIGHT_SCHEDULE
#define BH_CAP_RIGHT_SCHEDULE BH_CAP_RIGHT_CONTROL
#endif
#ifndef BH_CAP_RIGHT_FAULT_DOMAIN_MANAGE
#define BH_CAP_RIGHT_FAULT_DOMAIN_MANAGE BH_CAP_RIGHT_CONTROL
#endif

extern long bh_sys_nop(bh_syscall_ctx_t *ctx);
extern long bh_sys_thread_create(bh_syscall_ctx_t *ctx);
extern long bh_sys_thread_destroy(bh_syscall_ctx_t *ctx);
extern long bh_sys_sched_yield(bh_syscall_ctx_t *ctx);
extern long bh_sys_sched_sleep(bh_syscall_ctx_t *ctx);
extern long bh_sys_sched_set_priority(bh_syscall_ctx_t *ctx);
extern long bh_sys_sched_set_affinity(bh_syscall_ctx_t *ctx);
extern long bh_sys_vmm_map_page(bh_syscall_ctx_t *ctx);
extern long bh_sys_vmm_unmap_page(bh_syscall_ctx_t *ctx);
extern long bh_sys_cap_invoke(bh_syscall_ctx_t *ctx);
extern long bh_sys_endpoint_create(bh_syscall_ctx_t *ctx);
extern long bh_sys_endpoint_send(bh_syscall_ctx_t *ctx);
extern long bh_sys_endpoint_receive(bh_syscall_ctx_t *ctx);
extern long bh_sys_cap_delegate(bh_syscall_ctx_t *ctx);
extern long bh_sys_intent_set(bh_syscall_ctx_t *ctx);
extern long bh_sys_intent_get(bh_syscall_ctx_t *ctx);
extern long bh_sys_mem_alloc_class(bh_syscall_ctx_t *ctx);
extern long bh_sys_fault_domain_create(bh_syscall_ctx_t *ctx);
extern long bh_sys_fault_domain_destroy(bh_syscall_ctx_t *ctx);
extern long bh_sys_fault_domain_attach(bh_syscall_ctx_t *ctx);
extern long bh_sys_read(bh_syscall_ctx_t *ctx);
extern long bh_sys_write(bh_syscall_ctx_t *ctx);
extern long bh_sys_get_subsystem_caps(bh_syscall_ctx_t *ctx);
extern long bh_sys_thread_exit(bh_syscall_ctx_t *ctx);

static const bh_syscall_meta_t native_syscall_table[] = {
    [BH_SYS_NOP] = { BH_SYS_NOP, "nop", BH_SYS_CLASS_SYSTEM, 0, BH_SYSCALL_F_FAST, 0, BH_SYS_CAP_INDEX_NONE, CAP_TYPE_NONE, false, false, false, bh_sys_nop },
    [BH_SYS_THREAD_CREATE] = { BH_SYS_THREAD_CREATE, "thread_create", BH_SYS_CLASS_PROCESS, 1, BH_SYSCALL_F_CAP_REQUIRED | BH_SYSCALL_F_USER_READ | BH_SYSCALL_F_USER_WRITE, BH_CAP_RIGHT_RESOURCE_ALLOC, 0, CAP_TYPE_PROCESS, true, true, true, bh_sys_thread_create },
    [BH_SYS_THREAD_DESTROY] = { BH_SYS_THREAD_DESTROY, "thread_destroy", BH_SYS_CLASS_PROCESS, 1, BH_SYSCALL_F_CAP_REQUIRED, BH_CAP_RIGHT_PROCESS_MANAGE, 0, CAP_TYPE_THREAD, true, false, false, bh_sys_thread_destroy },
    [BH_SYS_SCHED_YIELD] = { BH_SYS_SCHED_YIELD, "sched_yield", BH_SYS_CLASS_SYSTEM, 0, BH_SYSCALL_F_FAST, 0, BH_SYS_CAP_INDEX_NONE, CAP_TYPE_NONE, false, false, false, bh_sys_sched_yield },
    [BH_SYS_SCHED_SLEEP] = { BH_SYS_SCHED_SLEEP, "sched_sleep", BH_SYS_CLASS_SYSTEM, 1, BH_SYSCALL_F_BLOCKING, 0, BH_SYS_CAP_INDEX_NONE, CAP_TYPE_NONE, false, false, false, bh_sys_sched_sleep },
    [BH_SYS_VMM_MAP_PAGE] = { BH_SYS_VMM_MAP_PAGE, "vmm_map_page", BH_SYS_CLASS_MEMORY, 1, BH_SYSCALL_F_CAP_REQUIRED | BH_SYSCALL_F_USER_READ, BH_CAP_RIGHT_MEMORY_MAP, 0, CAP_TYPE_MEMORY, true, true, false, bh_sys_vmm_map_page },
    [BH_SYS_VMM_UNMAP_PAGE] = { BH_SYS_VMM_UNMAP_PAGE, "vmm_unmap_page", BH_SYS_CLASS_MEMORY, 1, BH_SYSCALL_F_CAP_REQUIRED | BH_SYSCALL_F_USER_READ, BH_CAP_RIGHT_MEMORY_UNMAP, 0, CAP_TYPE_MEMORY, true, true, false, bh_sys_vmm_unmap_page },
    [BH_SYS_CAPABILITY_INVOKE] = { BH_SYS_CAPABILITY_INVOKE, "cap_invoke", BH_SYS_CLASS_CAPABILITY, 1, BH_SYSCALL_F_CAP_REQUIRED | BH_SYSCALL_F_USER_READ, BH_CAP_RIGHT_QUERY | BH_CAP_RIGHT_CRYPT_USE, 0, CAP_TYPE_NONE, true, true, false, bh_sys_cap_invoke },
    [BH_SYS_ENDPOINT_CREATE] = { BH_SYS_ENDPOINT_CREATE, "endpoint_create", BH_SYS_CLASS_IPC, 1, BH_SYSCALL_F_CAP_REQUIRED | BH_SYSCALL_F_USER_READ | BH_SYSCALL_F_USER_WRITE, BH_CAP_RIGHT_RESOURCE_ALLOC, 0, CAP_TYPE_PROCESS, true, true, true, bh_sys_endpoint_create },
    [BH_SYS_ENDPOINT_SEND] = { BH_SYS_ENDPOINT_SEND, "endpoint_send", BH_SYS_CLASS_IPC, 1, BH_SYSCALL_F_CAP_REQUIRED | BH_SYSCALL_F_BLOCKING | BH_SYSCALL_F_USER_READ, BH_CAP_RIGHT_ENDPOINT_SEND, 0, CAP_TYPE_ENDPOINT, true, true, false, bh_sys_endpoint_send },
    [BH_SYS_ENDPOINT_RECEIVE] = { BH_SYS_ENDPOINT_RECEIVE, "endpoint_receive", BH_SYS_CLASS_IPC, 1, BH_SYSCALL_F_CAP_REQUIRED | BH_SYSCALL_F_BLOCKING | BH_SYSCALL_F_USER_WRITE, BH_CAP_RIGHT_ENDPOINT_RECEIVE, 0, CAP_TYPE_ENDPOINT, true, false, true, bh_sys_endpoint_receive },
    [BH_SYS_CAPABILITY_DELEGATE] = { BH_SYS_CAPABILITY_DELEGATE, "cap_delegate", BH_SYS_CLASS_CAPABILITY, 1, BH_SYSCALL_F_CAP_REQUIRED | BH_SYSCALL_F_USER_READ | BH_SYSCALL_F_USER_WRITE, BH_CAP_RIGHT_DELEGATE, 0, CAP_TYPE_NONE, true, true, true, bh_sys_cap_delegate },
    [BH_SYS_SCHED_SET_PRIORITY] = { BH_SYS_SCHED_SET_PRIORITY, "sched_set_priority", BH_SYS_CLASS_SYSTEM, 1, BH_SYSCALL_F_CAP_REQUIRED | BH_SYSCALL_F_USER_READ, BH_CAP_RIGHT_SCHEDULE, 0, CAP_TYPE_SCHED, true, true, false, bh_sys_sched_set_priority },
    [BH_SYS_SCHED_SET_AFFINITY] = { BH_SYS_SCHED_SET_AFFINITY, "sched_set_affinity", BH_SYS_CLASS_SYSTEM, 1, BH_SYSCALL_F_CAP_REQUIRED | BH_SYSCALL_F_USER_READ, BH_CAP_RIGHT_SCHEDULE, 0, CAP_TYPE_SCHED, true, true, false, bh_sys_sched_set_affinity },
    [BH_SYS_INTENT_SET] = { BH_SYS_INTENT_SET, "intent_set", BH_SYS_CLASS_SYSTEM, 1, BH_SYSCALL_F_CAP_REQUIRED | BH_SYSCALL_F_USER_READ, BH_CAP_RIGHT_QUERY, 0, CAP_TYPE_PROCESS, true, true, false, bh_sys_intent_set },
    [BH_SYS_INTENT_GET] = { BH_SYS_INTENT_GET, "intent_get", BH_SYS_CLASS_SYSTEM, 1, BH_SYSCALL_F_CAP_REQUIRED | BH_SYSCALL_F_USER_READ | BH_SYSCALL_F_USER_WRITE, BH_CAP_RIGHT_QUERY, 0, CAP_TYPE_PROCESS, true, true, true, bh_sys_intent_get },
    [BH_SYS_MEM_ALLOC_CLASS] = { BH_SYS_MEM_ALLOC_CLASS, "mem_alloc_class", BH_SYS_CLASS_MEMORY, 1, BH_SYSCALL_F_CAP_REQUIRED | BH_SYSCALL_F_USER_READ | BH_SYSCALL_F_USER_WRITE, BH_CAP_RIGHT_RESOURCE_ALLOC, 0, CAP_TYPE_MEMORY, true, true, true, bh_sys_mem_alloc_class },
    [BH_SYS_FAULT_DOMAIN_CREATE] = { BH_SYS_FAULT_DOMAIN_CREATE, "fault_domain_create", BH_SYS_CLASS_SYSTEM, 1, BH_SYSCALL_F_CAP_REQUIRED | BH_SYSCALL_F_USER_READ | BH_SYSCALL_F_USER_WRITE, BH_CAP_RIGHT_RESOURCE_ALLOC, 0, CAP_TYPE_PROCESS, true, true, true, bh_sys_fault_domain_create },
    [BH_SYS_FAULT_DOMAIN_DESTROY] = { BH_SYS_FAULT_DOMAIN_DESTROY, "fault_domain_destroy", BH_SYS_CLASS_SYSTEM, 1, BH_SYSCALL_F_CAP_REQUIRED, BH_CAP_RIGHT_FAULT_DOMAIN_MANAGE, 0, CAP_TYPE_NONE, true, false, false, bh_sys_fault_domain_destroy },
    [BH_SYS_FAULT_DOMAIN_ATTACH] = { BH_SYS_FAULT_DOMAIN_ATTACH, "fault_domain_attach", BH_SYS_CLASS_SYSTEM, 1, BH_SYSCALL_F_CAP_REQUIRED | BH_SYSCALL_F_USER_READ, BH_CAP_RIGHT_FAULT_DOMAIN_MANAGE, 0, CAP_TYPE_NONE, true, true, false, bh_sys_fault_domain_attach },
    [BH_SYS_READ] = { BH_SYS_READ, "read", BH_SYS_CLASS_IO, 3, BH_SYSCALL_F_CAP_REQUIRED | BH_SYSCALL_F_BLOCKING | BH_SYSCALL_F_USER_WRITE, BH_CAP_RIGHT_READ, 0, CAP_TYPE_NONE, true, false, true, bh_sys_read },
    [BH_SYS_WRITE] = { BH_SYS_WRITE, "write", BH_SYS_CLASS_IO, 3, BH_SYSCALL_F_CAP_REQUIRED | BH_SYSCALL_F_BLOCKING | BH_SYSCALL_F_USER_READ, BH_CAP_RIGHT_WRITE, 0, CAP_TYPE_NONE, true, true, false, bh_sys_write },
    [BH_SYS_GET_SUBSYSTEM_CAPS] = { BH_SYS_GET_SUBSYSTEM_CAPS, "get_subsystem_caps", BH_SYS_CLASS_SYSTEM, 2, BH_SYSCALL_F_CAP_REQUIRED | BH_SYSCALL_F_USER_WRITE, BH_CAP_RIGHT_QUERY, 0, CAP_TYPE_NONE, true, false, true, bh_sys_get_subsystem_caps },
    [BH_SYS_THREAD_EXIT] = { BH_SYS_THREAD_EXIT, "thread_exit", BH_SYS_CLASS_PROCESS, 1, BH_SYSCALL_F_FAST, 0, BH_SYS_CAP_INDEX_NONE, CAP_TYPE_NONE, false, false, false, bh_sys_thread_exit },
};

const bh_personality_syscall_table_t native_personality = {
    .name = "native",
    .abi_version = 1,
    .max_syscall_nr = BH_SYS_THREAD_EXIT,
    .table = native_syscall_table
};
