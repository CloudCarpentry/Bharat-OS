#include "trap/syscall_context.h"
#include <bharat/uapi/syscall/nr.h>

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

static const bh_syscall_desc_t native_syscall_table[] = {
    [SYSCALL_NOP] = { SYSCALL_NOP, "nop", 0, BH_SYSCALL_F_FAST, 0, bh_sys_nop },
    [SYSCALL_THREAD_CREATE] = { SYSCALL_THREAD_CREATE, "thread_create", 1, BH_SYSCALL_F_CAP_REQUIRED, 0, bh_sys_thread_create },
    [SYSCALL_THREAD_DESTROY] = { SYSCALL_THREAD_DESTROY, "thread_destroy", 1, BH_SYSCALL_F_CAP_REQUIRED, 0, bh_sys_thread_destroy },
    [SYSCALL_SCHED_YIELD] = { SYSCALL_SCHED_YIELD, "sched_yield", 0, BH_SYSCALL_F_FAST, 0, bh_sys_sched_yield },
    [SYSCALL_SCHED_SLEEP] = { SYSCALL_SCHED_SLEEP, "sched_sleep", 1, BH_SYSCALL_F_BLOCKING, 0, bh_sys_sched_sleep },
    [SYSCALL_VMM_MAP_PAGE] = { SYSCALL_VMM_MAP_PAGE, "vmm_map_page", 1, BH_SYSCALL_F_CAP_REQUIRED, 0, bh_sys_vmm_map_page },
    [SYSCALL_VMM_UNMAP_PAGE] = { SYSCALL_VMM_UNMAP_PAGE, "vmm_unmap_page", 1, BH_SYSCALL_F_CAP_REQUIRED, 0, bh_sys_vmm_unmap_page },
    [SYSCALL_CAPABILITY_INVOKE] = { SYSCALL_CAPABILITY_INVOKE, "cap_invoke", 1, BH_SYSCALL_F_CAP_REQUIRED, 0, bh_sys_cap_invoke },
    [SYSCALL_ENDPOINT_CREATE] = { SYSCALL_ENDPOINT_CREATE, "endpoint_create", 1, BH_SYSCALL_F_FAST, 0, bh_sys_endpoint_create },
    [SYSCALL_ENDPOINT_SEND] = { SYSCALL_ENDPOINT_SEND, "endpoint_send", 1, BH_SYSCALL_F_BLOCKING | BH_SYSCALL_F_USER_READ, 0, bh_sys_endpoint_send },
    [SYSCALL_ENDPOINT_RECEIVE] = { SYSCALL_ENDPOINT_RECEIVE, "endpoint_receive", 1, BH_SYSCALL_F_BLOCKING | BH_SYSCALL_F_USER_WRITE, 0, bh_sys_endpoint_receive },
    [SYSCALL_CAPABILITY_DELEGATE] = { SYSCALL_CAPABILITY_DELEGATE, "cap_delegate", 1, BH_SYSCALL_F_FAST, 0, bh_sys_cap_delegate },
    [SYSCALL_SCHED_SET_PRIORITY] = { SYSCALL_SCHED_SET_PRIORITY, "sched_set_priority", 1, BH_SYSCALL_F_CAP_REQUIRED, 0, bh_sys_sched_set_priority },
    [SYSCALL_SCHED_SET_AFFINITY] = { SYSCALL_SCHED_SET_AFFINITY, "sched_set_affinity", 1, BH_SYSCALL_F_CAP_REQUIRED, 0, bh_sys_sched_set_affinity },
    [SYSCALL_INTENT_SET] = { SYSCALL_INTENT_SET, "intent_set", 1, BH_SYSCALL_F_FAST, 0, bh_sys_intent_set },
    [SYSCALL_INTENT_GET] = { SYSCALL_INTENT_GET, "intent_get", 1, BH_SYSCALL_F_FAST, 0, bh_sys_intent_get },
    [SYSCALL_MEM_ALLOC_CLASS] = { SYSCALL_MEM_ALLOC_CLASS, "mem_alloc_class", 1, BH_SYSCALL_F_CAP_REQUIRED, 0, bh_sys_mem_alloc_class },
    [SYSCALL_FAULT_DOMAIN_CREATE] = { SYSCALL_FAULT_DOMAIN_CREATE, "fault_domain_create", 1, BH_SYSCALL_F_CAP_REQUIRED, 0, bh_sys_fault_domain_create },
    [SYSCALL_FAULT_DOMAIN_DESTROY] = { SYSCALL_FAULT_DOMAIN_DESTROY, "fault_domain_destroy", 1, BH_SYSCALL_F_CAP_REQUIRED, 0, bh_sys_fault_domain_destroy },
    [SYSCALL_FAULT_DOMAIN_ATTACH] = { SYSCALL_FAULT_DOMAIN_ATTACH, "fault_domain_attach", 1, BH_SYSCALL_F_CAP_REQUIRED, 0, bh_sys_fault_domain_attach },
    [SYSCALL_READ] = { SYSCALL_READ, "read", 3, BH_SYSCALL_F_BLOCKING | BH_SYSCALL_F_USER_WRITE, 0, bh_sys_read },
    [SYSCALL_WRITE] = { SYSCALL_WRITE, "write", 3, BH_SYSCALL_F_BLOCKING | BH_SYSCALL_F_USER_READ, 0, bh_sys_write },
    [SYSCALL_GET_SUBSYSTEM_CAPS] = { SYSCALL_GET_SUBSYSTEM_CAPS, "get_subsystem_caps", 2, BH_SYSCALL_F_USER_WRITE, 0, bh_sys_get_subsystem_caps },
    [SYSCALL_THREAD_EXIT] = { SYSCALL_THREAD_EXIT, "thread_exit", 1, BH_SYSCALL_F_FAST, 0, bh_sys_thread_exit },
};

const bh_personality_syscall_table_t native_personality = {
    .name = "native",
    .abi_version = 1,
    .max_syscall_nr = 23,
    .table = native_syscall_table
};
