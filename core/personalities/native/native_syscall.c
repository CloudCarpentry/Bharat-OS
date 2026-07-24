#include "trap/syscall_context.h"
#include <bharat/uapi/syscall/bh_syscall_numbers.h>
#include <bharat/uapi/capability/rights.h>
#include "capability.h"


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
#include <kernel/syscall/native_syscall_table.inc>
};

const bh_personality_syscall_table_t native_personality = {
    .name = "native",
    .abi_version = BHARAT_SYSCALL_ABI_VERSION,
    .entry_count = BH_SYSCALL_COUNT,
    .table = native_syscall_table
};

_Static_assert(BH_SYSCALL_COUNT == (sizeof(native_syscall_table) / sizeof(native_syscall_table[0])),
               "syscall table and generated ABI count differ");
_Static_assert(BH_SYS_THREAD_EXIT < 256,
               "highest syscall exceeds reserved core range (256)");
